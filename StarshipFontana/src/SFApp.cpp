#include "SFApp.h"

SFApp::SFApp(std::shared_ptr<SFWindow> window) : fire(0), is_running(true), sf_window(window) {
  int canvas_w, canvas_h;
  SDL_GetRendererOutputSize(sf_window->getRenderer(), &canvas_w, &canvas_h);

  app_box = make_shared<SFBoundingBox>(Vector2(canvas_w, canvas_h), canvas_w, canvas_h);
  score = 0;

  background = make_shared<SFAsset>(SFASSET_BACKGROUND, sf_window);
  auto background_pos = Point2(canvas_w/2, canvas_h/2);
  background->SetPosition(background_pos);
  winScreen = make_shared<SFAsset>(SFASSET_WINSCREEN, sf_window);
  auto winScreen_pos = Point2(canvas_w/2, canvas_h/2);
  winScreen->SetPosition(winScreen_pos);
  player  = make_shared<SFAsset>(SFASSET_PLAYER, sf_window);
  auto player_pos = Point2(canvas_w/2, 22);
  player->SetPosition(player_pos);

  auto coin = make_shared<SFAsset>(SFASSET_COIN, sf_window);
  auto coin_pos  = Point2((canvas_w/4), 100);
  coin->SetPosition(coin_pos);
  coins.push_back(coin);
  coinCollected = false;

  const int walls_per_row = 6;
  const int rows_of_walls = 4;
  const int aliens_per_row = walls_per_row - 1;
  for (int j=0; j<rows_of_walls; j++) {
      if (j % 2 != 0) {
	for(int i=0; i<walls_per_row; i++) {
	  auto wall = make_shared<SFAsset>(SFASSET_WALL, sf_window);
	  auto wall_pos = Point2(40.0f+((canvas_w/walls_per_row) * i) + ((canvas_w/walls_per_row)/2), 64.0f+((canvas_h/rows_of_walls) * j));
	  wall->SetPosition(wall_pos);
	  walls.push_back(wall);
	}
	for(int i=0; i<aliens_per_row; i++) {
	  // place an alien at width/number_of_aliens * i
	  auto alien = make_shared<SFAsset>(SFASSET_ALIEN, sf_window);
	  auto pos   = Point2(40.0f+(canvas_w/walls_per_row) * (i+1), 64.0f+((canvas_h/rows_of_walls) * j));
	  alien->SetPosition(pos);
	  aliens.push_back(alien);
	}
      }
      else {
	for(int i=0; i<walls_per_row; i++) {
	  auto wall = make_shared<SFAsset>(SFASSET_WALL, sf_window);
	  auto wall_pos = Point2(40.0f+(canvas_w/walls_per_row) * i, 64.0f+((canvas_h/rows_of_walls) * j));
	  wall->SetPosition(wall_pos);
	  walls.push_back(wall);
	}
      }
  }
}

SFApp::~SFApp() {
}

/**
 * Handle all events that come from SDL.
 * These are timer or keyboard events.
 */
void SFApp::OnEvent(SFEvent& event) {
  SFEVENT the_event = event.GetCode();
  switch (the_event) {
  case SFEVENT_QUIT:
    is_running = false;
    break;
  case SFEVENT_UPDATE:
    OnUpdateWorld();
    OnRender();
    break;
  case SFEVENT_PLAYER_LEFT:
    player->GoWest();
    player->ChangePlayerSprite(3);
    directionPlayerFacing = 3;
    break;
  case SFEVENT_PLAYER_RIGHT:
    player->GoEast();
    player->ChangePlayerSprite(1);
    directionPlayerFacing = 1;
    break;
  case SFEVENT_PLAYER_UP:
    player->GoNorth();
    player->ChangePlayerSprite(0);
    directionPlayerFacing = 0;
    break;
  case SFEVENT_PLAYER_DOWN:
    player->GoSouth();
    player->ChangePlayerSprite(2);
    directionPlayerFacing = 2;
    break;
  case SFEVENT_FIRE:
    fire ++;
    if (directionPlayerFacing == 0) {
      FireProjectile(0);
    }
    else if (directionPlayerFacing == 1) {
      FireProjectile(1);
    }
    else if (directionPlayerFacing == 2) {
      FireProjectile(2);
    }
    else if (directionPlayerFacing == 3) {
      FireProjectile(3);
    }
    break;
  }
}

int SFApp::OnExecute() {
  // Execute the app
  SDL_Event event;
  while (SDL_WaitEvent(&event) && is_running) {
    // wrap an SDL_Event with our SFEvent
    SFEvent sfevent((const SDL_Event) event);
    // handle our SFEvent
    OnEvent(sfevent);
  }
}

void SFApp::OnUpdateWorld() {
  // Update projectile positions
  for(auto p: projectiles) {
    if (p->returnDirectionProjectileTraveling() == 0) {
      p->GoNorth();
    }
    else if (p->returnDirectionProjectileTraveling() == 1) {
      p->GoEast();
    }
    else if (p->returnDirectionProjectileTraveling() == 2) {
      p->GoSouth();
    }
    else if (p->returnDirectionProjectileTraveling() == 3) {
      p->GoWest();
    }
  }

  for(auto c: coins) {
    c->GoNorth();
  }

  // Update enemy positions
  enemyMoveCounter++;
  for(auto a : aliens) {
    if (enemyMoveCounter % 100 == 0) {
      a->GoEast();
      a->GoEast();
    }
    else if (enemyMoveCounter % 50 == 0){
      a->GoWest();
      a->GoWest();
    }
  }

  // Detect collisions
  for(auto p : projectiles) {
    for(auto a : aliens) {
      if(p->CollidesWith(a)) {
        p->HandleCollision();
        a->HandleCollision();
	score++;
	cout << score << endl;
      }
    }
  }

    for(auto w : walls) {
      if(player->CollidesWith(w)) {
	player->HandleCollision();
	w->HandleCollision();
      }
    }

    for(auto c : coins) {
      if(player->CollidesWith(c)) {
	c->HandleCollision();
	score += 100;
	cout << score << endl;
	//coinCollected = true;
      }
    }

  // remove coins
  list<shared_ptr<SFAsset>> tmpC;
  for(auto c : coins) {
    if(c->IsAlive()) {
      tmpC.push_back(c);
    }
  }
  coins.clear();
  coins = list<shared_ptr<SFAsset>>(tmpC);

  // remove dead aliens (the long way)
  list<shared_ptr<SFAsset>> tmpA;
  for(auto a : aliens) {
    if(a->IsAlive()) {
      tmpA.push_back(a);
    }
  }
  aliens.clear();
  aliens = list<shared_ptr<SFAsset>>(tmpA);

  // remove projectiles
  list<shared_ptr<SFAsset>> tmpP;
  for(auto p : projectiles) {
    if(p->IsAlive()) {
      tmpP.push_back(p);
    }
  }
  projectiles.clear();
  projectiles = list<shared_ptr<SFAsset>>(tmpP);



}


void SFApp::OnRender() {
  SDL_RenderClear(sf_window->getRenderer());

  // draw the player
    
  background->OnRender();
  
  if(player->IsAlive()) {player->OnRender();}

  for(auto p: projectiles) {
    if(p->IsAlive()) {p->OnRender();}
  }

  for(auto a: aliens) {
    if(a->IsAlive()) {a->OnRender();}
  }

  for(auto c: coins) {
    if(c->IsAlive()) { c->OnRender();}
  }

  for(auto w: walls) {
    w->OnRender();
  }

  if (coins.empty()) {winScreen->OnRender();}

  

  // Switch the off-screen buffer to be on-screen
  SDL_RenderPresent(sf_window->getRenderer());
}

void SFApp::FireProjectile(int directionShotIn) {
  auto pb = make_shared<SFAsset>(SFASSET_PROJECTILE, sf_window);
  auto v  = player->GetPosition();
  pb->SetPosition(v);
  pb->setDirectionProjectileTraveling(directionShotIn);
  projectiles.push_back(pb);
}
