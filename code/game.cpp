#include "base.h"

#define WIDTH  1080
#define HEIGHT 720

static bool g_running = true;

struct Bitmap {
    u32* data;
    u32  width;
    u32  height;
};

static void gameInit();
static void gameUpdate(float deltaSeconds);
static void render();

#include "breakout_win32.h"

static void drawSquare(u32 color, Vec2 center, Vec2 halfSize, Bitmap* bitmap) {
    int minX = (int)(center.x - halfSize.x);
    int minY = (int)(center.y - halfSize.y);
    int maxX = (int)(center.x + halfSize.x) + 1;
    int maxY = (int)(center.y + halfSize.y) + 1;

    if (minX >= (int)bitmap->width || minY >= (int)bitmap->height ||
        maxX <= 0 || maxY <= 0) {
        return;
    }

    minX = minX >= 0 ? minX : 0;
    minY = minY >= 0 ? minY : 0;
    maxX = maxX <= (int)bitmap->width  ? maxX : (int)bitmap->width;
    maxY = maxY <= (int)bitmap->height ? maxY : (int)bitmap->height;

    for (int y = minY; y < maxY; y++) {
        u32* p = &bitmap->data[y * bitmap->width + minX];
        for (int x = minX; x < maxX; x++) {
            *p++ = color;
        }
    }
}

static void drawCircle(u32 color, Vec2 center, float radius, Bitmap* bitmap) {
    int minX = (int)(center.x - radius);
    int minY = (int)(center.y - radius);
    int maxX = (int)(center.x + radius) + 1;
    int maxY = (int)(center.y + radius) + 1;

    if (minX >= (int)bitmap->width || minY >= (int)bitmap->height ||
        maxX < 0 || maxY < 0) {
        return;
    }

    minX = minX >= 0 ? minX : 0;
    minY = minY >= 0 ? minY : 0;
    maxX = maxX <= (int)bitmap->width  ? maxX : (int)bitmap->width;
    maxY = maxY <= (int)bitmap->height ? maxY : (int)bitmap->height;

    for (int y = minY; y < maxY; y++) {
        u32* p = &bitmap->data[y * bitmap->width + minX];
        for (int x = minX; x < maxX; x++) {
            Vec2 d = vec2(x,y) - center;
            float distanceSquared = dot(d,d);
            if (distanceSquared <= radius*radius) {
                *p = color;
            }
            p++;
        }
    }
}

struct Box {
    Vec2 center;
    Vec2 halfExtents;
};

struct Circle {
    Vec2  center;
    float radius;
};

struct Ball {
    Circle circle;
    Vec2   velocity;
    bool   alive;
    bool   ignoreTiles;
};

#define MAX_TILES 128
static Box tiles[MAX_TILES];
static int aliveTiles = 0;

static Box  player;
#define BALL_SPEED 400.0f
static Ball ball;

static bool startedRound = false;

static void resetPlayer() {
    player = {
        .center      = vec2(540, 600),
        .halfExtents = vec2(70, 5),
    };
}

static void resetBall() {
    ball = {
        .circle = {
            .center = vec2(540, 150),
            .radius = 8,
        },
        .velocity = vec2(0,0),
        .ignoreTiles = true,
    };
}

static void makeTileGrid() {
    int gridWidth  = 10;
    int gridHeight = 4;
    aliveTiles = gridWidth * gridHeight;
    ASSERT(aliveTiles <= MAX_TILES);

    float verticalPadding   = 40;
    float horizontalPadding = 20;
    float horizontalSpacing = 5;
    float verticalSpacing   = 5;
    Vec2 halfExtents = vec2(
        (WIDTH - horizontalPadding*2 - horizontalSpacing * (gridWidth-1)) / gridWidth * 0.5f,
        10.0f
    );
    
    for (int y = 0; y < gridHeight; y++) {
        Vec2 offset = vec2(
            horizontalPadding + halfExtents.x,
            verticalPadding + halfExtents.y + y * (halfExtents.y * 2 + verticalSpacing)
        );
        for (int x = 0; x < gridWidth; x++) {
            tiles[y * gridWidth + x] = (Box){
                .center      = offset,
                .halfExtents = halfExtents,
            };
            offset.x += halfExtents.x * 2 + horizontalSpacing;
        }
    }
}

static bool checkCollisionAndResolve(Box* box, Circle* circle, Vec2* hitNormal) {
    *hitNormal = vec2(0,0);
    Vec2 diff = circle->center - box->center;
    Vec2 d = diff;
    d = abs(d) - vec2(circle->radius);
    bool colliding  = (d.x <= box->halfExtents.x) &&
                      (d.y <= box->halfExtents.y);

    if (colliding) {
        Vec2 absDiff = box->halfExtents - abs(diff);
        Vec2 correction = vec2(0,0);
        if (absDiff.x <= absDiff.y) {
            float sign = diff.x >= 0 ? 1 : -1;
            correction.x = sign * (box->halfExtents.x + circle->radius) - diff.x;
        } else {
            float sign = diff.y >= 0 ? 1 : -1;
            correction.y = sign * (box->halfExtents.y + circle->radius) - diff.y; 
        }
        circle->center += correction;
        *hitNormal = normalize(correction);
    }
    
    return colliding;
}

void gameInit() {
    resetPlayer();
    resetBall();
    makeTileGrid();
}

void gameUpdate(float deltaSeconds) {
    if (!startedRound) {
        if (playerInput.left || playerInput.right || playerInput.a || playerInput.d) {
            startedRound = true;

            ball.velocity = vec2(0, BALL_SPEED);
        } else {
            return;
        }
    }

    float playerSpeedX = 0;
    if (playerInput.left || playerInput.a) {
        playerSpeedX -= 350;
    }
    if (playerInput.right || playerInput.d) {
        playerSpeedX += 350;
    }

    player.center.x += playerSpeedX * deltaSeconds;
    if (player.center.x < 0) {
        player.center.x = 0;
    } else if (player.center.x > g_window.width) {
        player.center.x = g_window.width;
    }

    ball.circle.center += deltaSeconds * ball.velocity;

    Vec2 hitNormal;
    if (checkCollisionAndResolve(&player, &ball.circle, &hitNormal)) {
        if (hitNormal.y < 0e-6f) {
            Vec2 dir = (ball.circle.center - player.center);
            dir.x *= 0.5f;
            dir = normalize(dir);
            ball.velocity = BALL_SPEED * dir;
        } else {
            ball.velocity = reflect(ball.velocity, hitNormal);
        }
        ball.ignoreTiles = false;
    }

    if (!ball.ignoreTiles) {
        for (int i = 0; i < aliveTiles; i++) {
            if (checkCollisionAndResolve(&tiles[i], &ball.circle, &hitNormal)) {
                aliveTiles--;
                tiles[i] = tiles[aliveTiles];
                ball.velocity = reflect(ball.velocity, hitNormal);
                break;
            }
        }
    }

    if (ball.circle.center.y - ball.circle.radius <= 0) {
        ball.velocity.y = fabs(ball.velocity.y);
    }
    if (ball.circle.center.x - ball.circle.radius <= 0) {
        ball.velocity.x = fabs(ball.velocity.x);
    } else if (ball.circle.center.x + ball.circle.radius >= g_window.width) {
        ball.velocity.x = -fabs(ball.velocity.x);
    }

    if (ball.circle.center.y + ball.circle.radius >= g_window.height) {
        startedRound = false;
        resetBall();
    }
}

void render() {
    { // clear backbuffer to black
        u32* p = g_backBuffer.bitmap.data;
        for (int y = 0; y < (int)g_backBuffer.bitmap.height; y++) {
            for (int x = 0; x < (int)g_backBuffer.bitmap.width; x++) {
                *p++ = 0xff000000;
            }
        }
    }

    for (int i = 0; i < aliveTiles; i++) {
        drawSquare(0xffff0000, tiles[i].center, tiles[i].halfExtents, &g_backBuffer.bitmap);
    }

    drawCircle(0xff00ff00, ball.circle.center, ball.circle.radius, &g_backBuffer.bitmap);
    drawSquare(0xff00ffff, player.center, player.halfExtents, &g_backBuffer.bitmap);
}
