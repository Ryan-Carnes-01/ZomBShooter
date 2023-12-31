#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <iostream>
#include <cmath>
#include <Custom/defs.h>

struct App{
    struct Entity{
        float x,y,dx,dy,rotation;
        float barrelOffsetX,barrelOffsetY;
        int w,h;
        int health;
        int fireDelay;
        SDL_Texture*texture;
        struct Entity*next;
    };
    struct Stage{
        struct Entity playerHead, *playerTail;
        struct Entity bulletHead, *bulletTail;
        struct Entity enemyHead, *enemyTail;
        SDL_Texture* bulletTexture,*enemyTexture,*playerTexture,*groundTexture;
    }; 
    
    SDL_Renderer* renderer; 
    SDL_Window* window;
    int keyboard[MAX_KEYBOARD_KEYS] = {0};
    struct Stage stage;
    int spawnTimer = 0;

    void initSDL(){
        int rendererflags = SDL_RENDERER_ACCELERATED;
        int windowflags = 0;

        if(SDL_Init(SDL_INIT_VIDEO) < 0){
            printf("Couldn't initialize SDL: %s\n", SDL_GetError());
            exit(1);
        }

        window = SDL_CreateWindow("SDL2 Game Testing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,SCREEN_HEIGHT,windowflags);
        if(!window){
            printf("failed to %d by %d window: %s\n",SCREEN_WIDTH,SCREEN_HEIGHT,SDL_GetError());
            exit(1);
        }

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        renderer = SDL_CreateRenderer(window,-1,rendererflags);
        if(!renderer){
            printf("failed to create renderer %s\n", SDL_GetError());
            exit(1);
        }
    }
    void initStage(){
        stage.playerTail = &stage.playerHead;
        stage.bulletTail = &stage.bulletHead;
        stage.enemyTail = &stage.enemyHead;
        stage.playerTail->next = NULL;
        stage.bulletTail->next = NULL;
        stage.enemyTail->next = NULL;

        stage.bulletTexture = loadTexture((char*)"imgs/bullet2.png");
        stage.enemyTexture = loadTexture((char*)"imgs/zombert.png");
        stage.playerTexture = loadTexture((char*)"imgs/gunguy.png");
        stage.groundTexture = loadTexture((char*)"imgs/ground.jpg");
        initPlayer();
    }    
    void initPlayer(){
        struct Entity* player = new struct Entity;
        stage.playerTail->next = player;
        stage.playerTail = player;
        stage.playerTail->next = NULL;

        player->x = PLAYER_START_X;
        player->y = PLAYER_START_Y;
        player->barrelOffsetX = 215;
        player->barrelOffsetY = 141;
        player->health = 100;
        player->fireDelay = 0;
        player->rotation = 0;
        player->texture = stage.playerTexture;
        SDL_QueryTexture(player->texture,NULL,NULL,&player->w,&player->h);
    }
    void initEnemy(){
        struct Entity* enemy = new struct Entity;
        stage.enemyTail->next = enemy;
        stage.enemyTail = enemy;
        stage.enemyTail->next = NULL;

        enemy->x = SCREEN_WIDTH;
        enemy->y = rand() % (SCREEN_HEIGHT-150);
        enemy->dx = -ENEMY_SPEED;
        enemy->dy = 0;
        enemy->health = 100;
        enemy->fireDelay = -1;
        enemy->rotation = 180;
        enemy->texture = stage.enemyTexture;
        SDL_QueryTexture(enemy->texture,NULL,NULL,&enemy->w,&enemy->h);
    }

    void destroySDL(){
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
    
    void prepareScene(){
        SDL_SetRenderDrawColor(renderer, 100,100,255,255);
        SDL_RenderClear(renderer);
    }
    void presentScene(){
        SDL_RenderPresent(renderer);
    }
    
    SDL_Texture* loadTexture(char*fname){
        SDL_Texture* texture;
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO, "Loading %s", fname);
        texture = IMG_LoadTexture(renderer,fname);
        return texture;
    }
    void blit(SDL_Texture*texture, float x, float y, float rotation,int width,int height){
        SDL_Rect dest;
        dest.x = x;
        dest.y = y;
        dest.w = width;
        dest.h = height;

        SDL_Point rotationCenter;
        rotationCenter.x = dest.w / 2;
        rotationCenter.y = (dest.h / 2);

        SDL_RenderCopyEx(renderer,texture,NULL,&dest,rotation,&rotationCenter,SDL_FLIP_NONE);
    }
    void doDraw(){
        drawBackground();
        drawPlayer();
        drawEnemy();
        drawBullets();
    }
    void drawBackground(){
        blit(stage.groundTexture,0,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
    }
    void drawPlayer(){
        Entity *p;
        for(p = stage.playerHead.next; p != NULL; p = p->next){
            blit(p->texture,p->x,p->y,p->rotation,PLAYER_TEXTURE_WIDTH,PLAYER_TEXTURE_HEIGHT);
        } 
    }
    void drawEnemy(){
        Entity *e;
        for(e = stage.enemyHead.next; e != NULL; e = e->next){
            blit(e->texture,e->x,e->y,e->rotation,ENEMY_TEXTURE_WIDTH,ENEMY_TEXTURE_HEIGHT);
        }
    }
    void drawBullets(){
	    Entity *b;
	    for (b = stage.bulletHead.next ; b != NULL ; b = b->next){
		    blit(b->texture, b->x, b->y,b->rotation,BULLET_TEXTURE_WIDTH,BULLET_TEXTURE_HEIGHT);
	    }
    }
    
    void doInput(){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT: 
                    destroySDL(); 
                    exit(0);
                    break;
                case SDL_KEYDOWN:
                    doKeyDown(&event.key);
                    break;
                case SDL_KEYUP:
                    doKeyUp(&event.key);
                    break;
                default:
                    break;
            }   
        }        
    }   
    void doKeyUp(SDL_KeyboardEvent *event){
	    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS){
		    keyboard[event->keysym.scancode] = 0;
	    }
    }
    void doKeyDown(SDL_KeyboardEvent *event){
	    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS){
		    keyboard[event->keysym.scancode] = 1;
	    }
    }

    void doLogic(){
        //doMouseRotation();
        spawnEnemy();
        doCollision();
        doPlayer();
        doEnemy();
        doBullets();
    }
    void doMouseRotation(){
        struct Entity* player = stage.playerTail;
        int mouseX,mouseY;
        SDL_GetMouseState(&mouseX,&mouseY);

        float dx = mouseX - (player->x);
        float dy = mouseY - (player->y);
        float angleRadians = atan2(dy,dx);

        float angleDegrees = angleRadians * (180.0f / M_PI);
        player->rotation = angleDegrees;
    }
    void spawnEnemy(){
        if(spawnTimer == 0){
            initEnemy();
            spawnTimer = SPAWN_TIMER;
        }
        spawnTimer--;
    }
    void doCollision(){
        struct Entity*b,*e;
        for(b = stage.bulletHead.next; b != NULL; b=b->next){
            for(e = stage.enemyHead.next; e != NULL; e=e->next){
                if(collision(b->x,b->y,b->w,b->h,e->x+80,e->y+80,e->w-70,e->h-140)){
                    e->health -= b->health;
                    b->health = 0;
                }
            }
        }
    }
    void doPlayer(){
        struct Entity*player = stage.playerTail;
        player->dx = player->dy = 0;

        if(player->fireDelay > 0){
            player->fireDelay--;
        }

        if(keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W]){
            if(player->y >= 0){
                player->dy = -PLAYER_SPEED;
            }
        }
        if(keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S]){
            if(player->y + player->h <= SCREEN_HEIGHT){
                player->dy = PLAYER_SPEED;
            }
        }
        if(keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_A]){
            if(player->x >= 0){
                player->dx = -PLAYER_SPEED;
            }
        }
        if(keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_D]){
            if(player->x + player->w <= SCREEN_WIDTH){
                player->dx = PLAYER_SPEED;
            }
        }
        if(keyboard[SDL_SCANCODE_SPACE] && player->fireDelay == 0){
            fireBullet();
            player->fireDelay = FIRE_DELAY;
        }

        player->x += player->dx;
        player->y += player->dy;
    }
    void fireBullet(){
        struct Entity*bullet = new struct Entity;
        struct Entity*player = stage.playerTail;
        stage.bulletTail->next = bullet;
        stage.bulletTail = bullet;
        stage.bulletTail->next = NULL;

        bullet->x = player->x + player->barrelOffsetX;
        bullet->y = player->y + player->barrelOffsetY;
        bullet->dx = BULLET_SPEED;
        bullet->dy = 0;
        bullet->health = BULLET_DAMAGE;
        bullet->rotation = 0;
        bullet->texture = stage.bulletTexture;
        SDL_QueryTexture(bullet->texture, NULL, NULL, &bullet->w, &bullet->h);
    }
    void doBullets(){
        struct Entity*b,*prev;
        prev = &stage.bulletHead;
        for(b = stage.bulletHead.next ; b != NULL ; b = b->next){
            b->x += b->dx;
            b->y += b->dy;

            if(b->x > SCREEN_WIDTH || b->health <= 0){
                if (b == stage.bulletTail){
				    stage.bulletTail = prev;
			    }
			    prev->next = b->next;
			    free(b);
			    b = prev;
            }
            prev = b;
        }
    }
    void doEnemy(){
        struct Entity*e,*prev;
        prev = &stage.enemyHead;
        for(e = stage.enemyHead.next; e != NULL; e = e->next){
            if(e->health <= 0 || e->x <= -(e->w)){
                if(e == stage.enemyTail){
                    stage.enemyTail = prev;
                }
                prev->next = e->next;
                free(e);
                e = prev;
            }
            prev = e;

            e->x += e->dx;
            e->y += e->dy;
        }
        
    }
    bool collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2){
        return (std::max(x1, x2) < std::min(x1 + w1, x2 + w2)) && (std::max(y1, y2) < std::min(y1 + h1, y2 + h2));
    }
};

int main(int argc, char*argv[]){
    struct App app;
    app.initSDL();
    app.initStage();
    while(1){
        app.prepareScene();
        app.doInput();
        app.doLogic();
        app.doDraw();
        app.presentScene();
        SDL_Delay(FRAME_DELAY);
    }
    return 0;
}