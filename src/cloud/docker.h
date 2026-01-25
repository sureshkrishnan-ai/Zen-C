// Zen-C Cloud Build - Dockerfile Generation

#ifndef ZC_DOCKER_H
#define ZC_DOCKER_H

#include "cloud_config.h"

// Generate Dockerfile in output_dir
// Returns 1 on success, 0 on failure
int docker_generate_dockerfile(CloudConfig *cfg, const char *binary_name,
                               const char *output_dir);

// Build Docker image using docker build
// Returns 1 on success, 0 on failure
int docker_build_image(CloudConfig *cfg, const char *context_dir, int verbose);

// Push Docker image to registry
// Returns 1 on success, 0 on failure
int docker_push_image(CloudConfig *cfg, int verbose);

// Check if Docker is available
int docker_is_available(void);

#endif // ZC_DOCKER_H
