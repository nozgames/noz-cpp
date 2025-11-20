//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

float Repeat(float t, float length) {
    return Clamp(t - Floor(t / length) * length, 0.0f, length);
}

float SmoothDamp(float current, float target, float& current_velocity, float smooth_time, float max_speed, float delta_time) {
    smooth_time = Max(0.0001f, smooth_time);
    float omega = 2.0f / smooth_time;
    float x = omega * delta_time;
    float exp = 1.0f / (1.0f + x + 0.48F * x * x + 0.235F * x * x * x);
    float change = current - target;
    float original_to = target;

    // Clamp maximum speed
    float max_change = max_speed * smooth_time;
    change = Clamp(change, -max_change, max_change);

    float temp = (current_velocity - omega * change) * delta_time;
    current_velocity = (current_velocity - omega * temp) * exp;
    float output = target + (change + temp) * exp;

    // Prevent overshooting
    if (original_to - current > 0.0f == output > original_to) {
        output = original_to;
        current_velocity = (output - original_to) / delta_time;
    }

    return output;
}

Vec2 SmoothDamp(const Vec2& current, const Vec2& target, Vec2& current_velocity, float smooth_time, float max_speed, float delta_time) {
    smooth_time = Max(0.0001f, smooth_time);
    float omega = 2.0f / smooth_time;

    float x = omega * delta_time;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change_x = current.x - target.x;
    float change_y = current.y - target.y;
    Vec2 original_to = target;

    // Clamp maximum speed
    float maxChange = max_speed * smooth_time;

    float maxChangeSq = maxChange * maxChange;
    float sqDist = change_x * change_x + change_y * change_y;
    if (sqDist > maxChangeSq) {
        float mag = Sqrt(sqDist);
        change_x = change_x / mag * maxChange;
        change_y = change_y / mag * maxChange;
    }

    float temp_x = (current_velocity.x + omega * change_x) * delta_time;
    float temp_y = (current_velocity.y + omega * change_y) * delta_time;

    current_velocity.x = (current_velocity.x - omega * temp_x) * exp;
    current_velocity.y = (current_velocity.y - omega * temp_y) * exp;

    float output_x = (current.x - change_x) + (change_x + temp_x) * exp;
    float output_y = (current.y - change_y) + (change_y + temp_y) * exp;

    // Prevent overshooting
    float origMinusCurrent_x = original_to.x - current.x;
    float origMinusCurrent_y = original_to.y - current.y;
    float outMinusOrig_x = output_x - original_to.x;
    float outMinusOrig_y = output_y - original_to.y;

    if (origMinusCurrent_x * outMinusOrig_x + origMinusCurrent_y * outMinusOrig_y > 0) {
        output_x = original_to.x;
        output_y = original_to.y;

        current_velocity.x = (output_x - original_to.x) / delta_time;
        current_velocity.y = (output_y - original_to.y) / delta_time;
    }

    return Vec2{output_x, output_y};
}
