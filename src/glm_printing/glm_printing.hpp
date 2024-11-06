#ifndef GLM_PRINTING_HPP
#define GLM_PRINTING_HPP

#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>

// Template function to print glm vectors
template <typename T, size_t N> void print_vec(const glm::vec<N, T> &vec);

// Template function to print glm matrices
template <typename T, size_t R, size_t C> void print_mat(const glm::mat<R, C, T> &mat);

#include "glm_printing.tpp" // Include the template implementation

#endif // GLM_PRINTING_HPP
