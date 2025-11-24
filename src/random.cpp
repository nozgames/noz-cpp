//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <random>

static std::random_device g_rd;
static std::mt19937 g_gen;

void InitRandom()
{
    g_gen.seed(g_rd());
}

float RandomFloat()
{
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(g_gen);
}

float RandomFloat(float min, float max)
{
    std::uniform_real_distribution<float> dis(min, max);
    return dis(g_gen);
}

int RandomInt(int min, int max)
{
    std::uniform_int_distribution<int> dis(min, max);
    return dis(g_gen);
}

bool RandomBool()
{
    static std::uniform_int_distribution<int> dis(0, 1);
    return dis(g_gen) == 1;
}

bool RandomBool(float probability) {
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(g_gen) < probability;
}

Vec2 RandomVec2(const Vec2& min, const Vec2& max) {
    return Vec2{
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y)
    };
}
