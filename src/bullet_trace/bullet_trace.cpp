#include "bullet_trace.hpp"

/*
 * A bullet trace is the trace of a bullet fired from a gun
 * when it exits the muzzle it is infinitelesly small, over time
 * it scales up until it reaches a max height, then it continues
 * moving at the same rate.
 */
BulletTrace::BulletTrace(glm::vec3 start_pos, glm::vec3 travel_dir, float travel_speed)
    : start_pos(start_pos), travel_dir(glm::normalize(travel_dir)), travel_speed(travel_speed) {}

IndexedVertexPositions BulletTrace::get_trace_rect(double delta_time_sec, glm::vec3 cam_pos) {
    time_since_fire_sec += delta_time_sec;
    float height = time_since_fire_sec * travel_speed;
    float offset = 0;

    if (height >= max_height_of_trace) {
        height = max_height_of_trace;
        time_since_full_scale_sec += delta_time_sec;
        // we need to continue moving at the same rate that the width was moving
        // as we updated the height, if the aspect ratio of x:y is 5:1, then
        // when y changes by c, then x changes by 5 * c thus we must multiply
        // by the ratio here to maintain the same speed
        offset += aspect_ratio_of_trace_texture * time_since_full_scale_sec * travel_speed;
    }

    // note that width is really length, but we think in terms
    // of an axis aligned setup to simplify the throught process
    float width = aspect_ratio_of_trace_texture * height;

    // calculate the center of the rectangle using start_pos and travel_dir
    glm::vec3 center_of_rect = start_pos + travel_dir * (width / 2.0f + offset); // half height + offset for positioning

    // calculate the direction from the camera to the center of the rectangle
    glm::vec3 cam_to_center = glm::normalize(center_of_rect - cam_pos);

    // calculate the height direction (perpendicular to travel_dir and cam_to_center)
    glm::vec3 height_dir = glm::normalize(glm::cross(travel_dir, cam_to_center));

    return {generate_rectangle_indices(),
            generate_rectangle_vertices_3d(center_of_rect, travel_dir, height_dir, width, height)};
}
