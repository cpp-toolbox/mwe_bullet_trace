#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "mouse/mouse.hpp"
#include "shader_standard/shader_standard.hpp"
#include "window/window.hpp"
#include "shader_cache/shader_cache.hpp"
#include "batcher/generated/batcher.hpp"
#include "vertex_geometry/vertex_geometry.hpp"
#include "draw_info/draw_info.hpp"
#include "transform/transform.hpp"
#include "glfw_lambda_callback_manager/glfw_lambda_callback_manager.hpp"
#include "glm_printing/glm_printing.hpp"
#include "fps_camera/fps_camera.hpp"
#include "bullet_trace/bullet_trace.hpp"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr if needed
#include <cstdlib>              // For std::rand and std::srand
#include <ctime>                // For std::time

class Minigun {
  public:
    glm::vec3 position;
    glm::vec3 fire_direction;         // Direction to fire
    float fire_rate;                  // Bullets per second
    float time_since_last_fire = 0;   // Time tracker for firing
    float randomness_factor = 0.1f;   // Factor for randomness
    std::vector<BulletTrace> bullets; // List of active bullet traces

    Minigun(glm::vec3 direction, glm::vec3 position, float rate)
        : fire_direction(glm::normalize(direction)), position(position), fire_rate(rate) {
        std::srand(static_cast<unsigned>(std::time(0))); // Seed random number generator
    }

    std::vector<IndexedVertexPositions> update(float delta_time_sec, glm::vec3 cam_pos) {
        time_since_last_fire += delta_time_sec;

        // Check if it's time to fire a new bullet
        while (time_since_last_fire >= (1.0f / fire_rate)) {
            // Create a random offset direction
            glm::vec3 random_offset = get_random_direction();
            glm::vec3 bullet_direction = glm::normalize(fire_direction + random_offset);
            bullets.emplace_back(position, bullet_direction, 1.0f); // Fire a bullet with random direction
            time_since_last_fire -= (1.0f / fire_rate);             // Reset the timer
        }

        // Prepare to collect drawing positions for each active bullet
        std::vector<IndexedVertexPositions> drawing_positions;

        // Update each bullet and collect its drawing positions
        for (auto &bullet : bullets) {
            IndexedVertexPositions trace_data = bullet.get_trace_rect(delta_time_sec, cam_pos);
            drawing_positions.push_back(trace_data); // Add the drawing data for this bullet
        }

        // Clean up old bullets if needed
        cleanup_bullets();

        return drawing_positions;
    }

  private:
    void cleanup_bullets() {
        // Implement logic to remove bullets that are no longer visible or needed
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                     [](const BulletTrace &bullet) {
                                         return bullet.time_since_fire_sec > 5.0f; // Example lifetime of 5 seconds
                                     }),
                      bullets.end());
    }

    glm::vec3 get_random_direction() {
        // Generate a random vector with small offsets
        float random_x = ((std::rand() % 100) / 100.0f - 0.5f) * randomness_factor;
        float random_y = ((std::rand() % 100) / 100.0f - 0.5f) * randomness_factor;
        float random_z = ((std::rand() % 100) / 100.0f - 0.5f) * randomness_factor;

        return glm::vec3(random_x, random_y, random_z);
    }
};

unsigned int SCREEN_WIDTH = 640;
unsigned int SCREEN_HEIGHT = 480;

static void error_callback(int error, const char *description) { fprintf(stderr, "Error: %s\n", description); }

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

// Wrapper that automatically creates a lambda for member functions
template <typename T, typename R, typename... Args> auto wrap_member_function(T &obj, R (T::*f)(Args...)) {
    // Return a std::function that wraps the member function in a lambda
    return std::function<R(Args...)>{[&obj, f](Args &&...args) { return (obj.*f)(std::forward<Args>(args)...); }};
}

int main() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("mwe_shader_cache_logs.txt", true);
    file_sink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};

    LiveInputState live_input_state;
    bool fullscreen = false;
    GLFWwindow *window = initialize_glfw_glad_and_return_window(&SCREEN_WIDTH, &SCREEN_HEIGHT, "glfw window",
                                                                fullscreen, false, false, &live_input_state);

    FPSCamera camera(glm::vec3(0, 0, 3), 50, SCREEN_WIDTH, SCREEN_HEIGHT, 90, 0.1, 50);
    std::function<void(unsigned int)> char_callback = [](unsigned int _) {};
    std::function<void(int, int, int, int)> key_callback = [](int _, int _1, int _2, int _3) {};
    std::function<void(double, double)> mouse_pos_callback = wrap_member_function(camera, &FPSCamera::mouse_callback);
    std::function<void(int, int, int)> mouse_button_callback = [](int _, int _1, int _2) {};
    GLFWLambdaCallbackManager glcm(window, char_callback, key_callback, mouse_pos_callback, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide and capture the mouse

    std::vector<ShaderType> requested_shaders = {ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR,
                                                 /*ShaderType::TRANSFORM_V_WITH_TEXTURES,*/
                                                 ShaderType::CWL_V_TRANSFORMATION_WITH_TEXTURES,
                                                 ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR};

    ShaderCache shader_cache(requested_shaders, sinks);
    Batcher batcher(shader_cache);

    int width, height;

    glm::vec4 color = glm::vec4(1, 1, .25, 1);
    shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR, ShaderUniformVariable::RGBA_COLOR,
                             color);

    glm::vec3 fire_direction = glm::vec3(1, 0, 0); // Adjust as needed for your game
    float fire_rate = 10.0f;                       // 10 bullets per second
    Minigun minigun(glm::vec3(1, 1, 1), fire_direction, fire_rate);

    double previous_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        double current_time = glfwGetTime();
        double delta_time = current_time - previous_time;
        previous_time = current_time;

        camera.process_input(window, delta_time);

        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto bullet_positions = minigun.update(delta_time, camera.transform.position);

        glm::mat4 projection = camera.get_projection_matrix();
        glm::mat4 view = camera.get_view_matrix();
        /*print_mat<float, 4, 4>(view);*/

        shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR,
                                 ShaderUniformVariable::CAMERA_TO_CLIP, projection);
        shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR,
                                 ShaderUniformVariable::WORLD_TO_CAMERA, view);
        shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_WITH_SOLID_COLOR,
                                 ShaderUniformVariable::LOCAL_TO_WORLD, glm::mat4(1.0));

        for (const auto &bullet_position : bullet_positions) {
            batcher.cwl_v_transformation_with_solid_color_shader_batcher.queue_draw(bullet_position.indices,
                                                                                    bullet_position.xyz_positions);
        }

        auto unit_square = generate_square_vertices(0, 0, 1);
        auto square_indices = generate_rectangle_indices();

        Transform unit_square_transform;
        unit_square_transform.position = glm::vec3(0, 0, -1);

        batcher.cwl_v_transformation_with_solid_color_shader_batcher.queue_draw(square_indices, unit_square);
        /*batcher.cwl_v_transformation_with_solid_color_shader_batcher.queue_draw(ivp.indices, ivp.xyz_positions);*/
        batcher.cwl_v_transformation_with_solid_color_shader_batcher.draw_everything();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
