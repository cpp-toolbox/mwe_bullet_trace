// Template for printing glm vectors
template <typename T, size_t N> void print_vec(const glm::vec<N, T> &vec) {
    std::cout << "(";
    for (size_t i = 0; i < N; ++i) {
        std::cout << std::fixed << std::setprecision(4) << vec[i];
        if (i < N - 1)
            std::cout << ", ";
    }
    std::cout << ")" << std::endl;
}

// Template for printing glm matrices with a bounding box
template <typename T, size_t R, size_t C> void print_mat(const glm::mat<R, C, T> &mat) {
    // Print the top boundary of the box
    std::cout << "+";
    for (size_t i = 0; i < C; ++i) {
        std::cout << "--------"; // Adjusted width for matrix elements
    }
    std::cout << "+" << std::endl;

    // Print the matrix rows
    for (size_t i = 0; i < R; ++i) {
        std::cout << "|"; // Start of the row
        for (size_t j = 0; j < C; ++j) {
            std::cout << " " << std::fixed << std::setprecision(4) << std::setw(8) << mat[i][j] << " ";
        }
        std::cout << "|" << std::endl;
    }

    // Print the bottom boundary of the box
    std::cout << "+";
    for (size_t i = 0; i < C; ++i) {
        std::cout << "--------"; // Adjusted width for matrix elements
    }
    std::cout << "+" << std::endl;
}
