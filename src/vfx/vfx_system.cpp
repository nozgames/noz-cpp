//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0
float evaluate_curve(float t, VfxCurveType curve)
{
    // Clamp t to [0,1]
    t = std::clamp(t, 0.0f, 1.0f);

    switch (curve)
    {
    case VFX_CURVE_TYPE_LINEAR:
        return t;

    case VFX_CURVE_TYPE_EASE_IN:
        return t * t;

    case VfxCurveType::ease_out:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case VfxCurveType::ease_in_out:
        if (t < 0.5f)
            return 2.0f * t * t;
        else
            return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);

    case VfxCurveType::quadratic:
        return t * t;

    case VFX_CURVE_TYPE_CUBIC:
        return t * t * t;

    case VFX_CURVE_TYPE_SINE:
        return std::sin(t * 3.14159265359f * 0.5f);  // 0 to pi/2

    case VfxCurveType::custom:
    default:
        return t;  // Fallback to linear
    }
}

// Helper functions
float evaluate_constant(const VfxValue& value)
{
    if (std::holds_alternative<VfxConstantValue>(value))
    {
        return std::get<VfxConstantValue>(value).value;
    }
    else if (std::holds_alternative<VfxParsedFloat>(value))
    {
        const auto& range = std::get<VfxParsedFloat>(value);
        return (range.min + range.max) * 0.5f; // Return midpoint
    }
    return 0.0f;
}

std::pair<float, float> evaluate_range(const VfxValue& value)
{
    if (std::holds_alternative<VfxConstantValue>(value))
    {
        float val = std::get<VfxConstantValue>(value).value;
        return {val, val};
    }
    else if (std::holds_alternative<VfxParsedFloat>(value))
    {
        const auto& range = std::get<VfxParsedFloat>(value);
        return {range.min, range.max};
    }
    return {0.0f, 0.0f};
}


#endif