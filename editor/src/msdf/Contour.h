//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "Edge.h"

namespace noz::msdf
{
    struct Contour
    {
        ~Contour();

        void bounds(double& l, double& b, double& r, double& t);
        int winding();

        std::vector<Edge*> edges;
    };
}
