#ifndef BULLET_TRACE_HPP
#define BULLET_TRACE_HPP

#include <glm/glm.hpp>
#include "sbpt_generated_includes.hpp"

/*
 * A bullet trace is the trace of a bullet fired from a gun
 * when it exits the muzzle it is infinitelesly small, over time
 * it scales up until it reaches a max height, then it continues
 * moving at the same rate.
 */
class BulletTrace {
  public:
    Transform transform;

    glm::vec3 start_pos;
    glm::vec3 travel_dir;
    float time_since_fire_sec = 0;
    float time_since_full_scale_sec = 0;
    float max_height_of_trace = 1.0 / 25;
    float travel_speed = 1;
    glm::vec3 center_of_bullet_trace = glm::vec3(0, 0, 0);
    float aspect_ratio_of_trace_texture = 512.0 / 32.0;

    BulletTrace(glm::vec3 start_pos, glm::vec3 travel_dir, float travel_speed);

    draw_info::IndexedVertexPositions get_trace_rect(double delta_time_sec, glm::vec3 cam_pos);
};

#endif // BULLET_TRACE_HPP
