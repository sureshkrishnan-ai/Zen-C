// Zen-C Cloud Build System
// Ballerina-style cloud-native build support with Docker and Kubernetes

#ifndef ZC_CLOUD_H
#define ZC_CLOUD_H

#include "toml.h"
#include "cloud_config.h"
#include "docker.h"
#include "kubernetes.h"

// Main cloud build entry point
// Called after successful binary compilation
// Returns 1 on success, 0 on failure
int cloud_build(const char *binary_path, const char *binary_name,
                int cloud_docker, int cloud_k8s, int cloud_push,
                const char *registry_url, const char *image_tag,
                const char *config_file, int verbose);

#endif // ZC_CLOUD_H
