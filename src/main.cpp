#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "window/window.hpp"
#include "shader_cache/shader_cache.hpp"
#include "batcher/generated/batcher.hpp"
#include "vertex_geometry/vertex_geometry.hpp"
#include "draw_info/draw_info.hpp"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>

/*
 * A bullet trace is the trace of a bullet fired from a gun
 * when it exits the muzzle it is infinitelesly small, over time
 * it scales up until it reaches a max height, then it continues
 * moving at the same rate.
 */
class BulletTrace {
  public:
    glm::vec3 start_pos;
    glm::vec3 travel_dir;
    float time_since_fire_sec = 0;
    float time_since_full_scale_sec = 0;
    float max_height_of_trace = 1.0 / 25;
    float travel_speed = 1;
    // x / y which can be thought of as a function where you pass in
    // the height and it yields the width to maintain the ratio
    float aspect_ratio_of_trace_texture = 512.0 / 32.0;

    BulletTrace(glm::vec3 start_pos, glm::vec3 travel_dir) : start_pos(start_pos), travel_dir(travel_dir) {}

    IndexedVertexPositions get_trace_rect(double delta_time_sec) {
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

        float center_of_rect = width / 2 + offset;

        /*std::cout << center_of_rect << ", " << height << ", " << width << std::endl;*/

        return {generate_rectangle_indices(),
                generate_rectangle_vertices(start_pos.x + center_of_rect, start_pos.y + 0, width, height)};

        // vertex geometry create a rect at (center_of_rect, 0, width, height)
        // we do the vertical billboarded approach by considering it at as 2d problem
        // finding the connecting vector and computing the transform to make it perpendicular
        // but before doing any of that, just make sure that the above works properly
    }
};

unsigned int SCREEN_WIDTH = 640;
unsigned int SCREEN_HEIGHT = 480;

static void error_callback(int error, const char *description) { fprintf(stderr, "Error: %s\n", description); }

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

struct OpenGLDrawingData {
    GLuint vbo_name;
    GLuint ibo_name;
    GLuint vao_name;
};

std::vector<glm::vec3> vertices_left = {
    glm::vec3(-0.8f, 0.5f, 0.0f),  // top right of left square
    glm::vec3(-0.8f, -0.5f, 0.0f), // bottom right of left square
    glm::vec3(-0.4f, -0.5f, 0.0f), // bottom left of left square
    glm::vec3(-0.4f, 0.5f, 0.0f)   // top left of left square
};

std::vector<glm::vec3> vertices_right = {
    glm::vec3(0.4f, 0.5f, 0.0f),  // top right of right square
    glm::vec3(0.4f, -0.5f, 0.0f), // bottom right of right square
    glm::vec3(0.8f, -0.5f, 0.0f), // bottom left of right square
    glm::vec3(0.8f, 0.5f, 0.0f)   // top left of right square
};

std::vector<unsigned int> indices = {
    // note that we start from 0!
    0, 1, 3, // first Triangle
    1, 2, 3  // second Triangle
};

int main() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("mwe_shader_cache_logs.txt", true);
    file_sink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};

    LiveInputState live_input_state;
    bool fullscreen = true;
    GLFWwindow *window = initialize_glfw_glad_and_return_window(&SCREEN_WIDTH, &SCREEN_HEIGHT, "glfw window", true,
                                                                false, false, &live_input_state);

    std::vector<ShaderType> requested_shaders = {ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR,
                                                 ShaderType::TRANSFORM_V_WITH_TEXTURES};
    ShaderCache shader_cache(requested_shaders, sinks);
    Batcher batcher(shader_cache);

    int width, height;

    glm::vec4 color = glm::vec4(1, 1, .25, 1);
    shader_cache.set_uniform(ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR, ShaderUniformVariable::RGBA_COLOR, color);

    BulletTrace bullet_trace(glm::vec3(-0.5, 0, 0), glm::vec3(0, 1, 0));

    double elapsed_time_sec = 0;
    double time_since_last_fire_sec = 0;
    double previous_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        double current_time = glfwGetTime();
        double delta_time = current_time - previous_time;
        previous_time = current_time;

        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto ivp = bullet_trace.get_trace_rect(delta_time);

        if (time_since_last_fire_sec > 0.25) {
            bullet_trace.start_pos = glm::vec3(-0.75, std::sin(elapsed_time_sec), 0);
            ;
            bullet_trace.travel_speed = 1.2 + std::sin(elapsed_time_sec);
            bullet_trace.time_since_fire_sec = 0;
            bullet_trace.time_since_full_scale_sec = 0;
            time_since_last_fire_sec = 0;
        }

        batcher.absolute_position_with_solid_color_shader_batcher.queue_draw(ivp.indices, ivp.xyz_positions);
        batcher.absolute_position_with_solid_color_shader_batcher.draw_everything();
        time_since_last_fire_sec += delta_time;
        elapsed_time_sec += delta_time;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
