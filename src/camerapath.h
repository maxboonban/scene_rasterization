#pragma once
#include <glm/glm.hpp>
#include <vector>

// A simple cubic Bézier evaluator for positions.
// Given control points P0,P1,P2,P3 and parameter t in [0,1].
inline glm::vec3 bezierCubic(const glm::vec3 &p0,
                             const glm::vec3 &p1,
                             const glm::vec3 &p2,
                             const glm::vec3 &p3,
                             float t)
{
    float u = 1.f - t;
    return u*u*u * p0 +
           3.f * u*u * t * p1 +
           3.f * u * t*t * p2 +
           t*t*t * p3;
}

// ---------------------------------------------------------------------------
// CameraPath: stores a poly-Bezier camera trajectory.
// For now we use a simple hard-coded curve with 4 control points,
// but the structure allows extension to multiple segments.
// ---------------------------------------------------------------------------

class CameraPath {
public:
    // Control points for a single cubic Bezier.
    std::vector<glm::vec3> pts;   // size = 4
    float durationSec = 5.f;      // how long to traverse the curve
    float timer = 0.f;            // internal path timer

    void reset() { timer = 0.f; }

    // dt: timestep from timerEvent
    // returns (pos, finishedFlag)
    glm::vec3 sample(float dt, bool &finished) {
        timer += dt;
        if (timer > durationSec) {
            timer = durationSec;
            finished = true;
        } else {
            finished = false;
        }

        float t = glm::clamp(timer / durationSec, 0.f, 1.f);
        return bezierCubic(pts[0], pts[1], pts[2], pts[3], t);
    }
};
