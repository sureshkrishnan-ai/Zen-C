// Zen-C Cloud Build System - Main Entry Point

#include "cloud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use real malloc for cloud module
#undef malloc
#undef free
#undef realloc
#undef calloc

int cloud_build(const char *binary_path, const char *binary_name,
                int cloud_docker, int cloud_k8s, int cloud_push,
                const char *registry_url, const char *image_tag,
                const char *config_file, int verbose) {
    (void)binary_path; // Unused for now

    // Determine config file path (default: zen.toml)
    const char *toml_path = config_file ? config_file : "zen.toml";

    // Try to load configuration
    CloudConfig *cfg = cloud_config_load(toml_path);

    if (cfg) {
        if (verbose) {
            printf("[cloud] Loaded configuration from %s\n", toml_path);
        }
    } else {
        // Use defaults if config file not found
        if (config_file) {
            // User specified a config file but it wasn't found
            fprintf(stderr, "[cloud] Warning: Config file '%s' not found, using defaults\n", config_file);
        } else if (verbose) {
            printf("[cloud] No zen.toml found, using defaults\n");
        }
        cfg = cloud_config_defaults(binary_name);
    }

    // Merge CLI options (they override TOML)
    cloud_config_merge_cli(cfg, registry_url, image_tag, binary_name);

    if (verbose) {
        cloud_config_print(cfg);
    }

    int success = 1;

    // Docker build
    if (cloud_docker) {
        printf("[cloud] === Docker Build ===\n");

        // Generate Dockerfile
        if (!docker_generate_dockerfile(cfg, binary_name, ".")) {
            fprintf(stderr, "[cloud] Failed to generate Dockerfile\n");
            success = 0;
            goto cleanup;
        }

        // Build image
        if (!docker_build_image(cfg, ".", verbose)) {
            fprintf(stderr, "[cloud] Failed to build Docker image\n");
            success = 0;
            goto cleanup;
        }
    }

    // Kubernetes manifests
    if (cloud_k8s) {
        printf("[cloud] === Kubernetes Manifests ===\n");

        if (!k8s_generate_manifests(cfg, "k8s")) {
            fprintf(stderr, "[cloud] Failed to generate Kubernetes manifests\n");
            success = 0;
            goto cleanup;
        }
    }

    // Push to registry
    if (cloud_push) {
        printf("[cloud] === Docker Push ===\n");

        if (!cfg->registry_url || strlen(cfg->registry_url) == 0) {
            fprintf(stderr, "[cloud] Error: Registry URL required for push. Use --registry=<url> or set in config\n");
            success = 0;
            goto cleanup;
        }

        if (!docker_push_image(cfg, verbose)) {
            fprintf(stderr, "[cloud] Failed to push Docker image\n");
            success = 0;
            goto cleanup;
        }
    }

    // Summary
    if (success) {
        printf("\n[cloud] === Build Complete ===\n");

        if (cloud_docker) {
            char *image = cloud_config_image_name(cfg);
            printf("[cloud] Docker image: %s\n", image);
            free(image);
        }

        if (cloud_k8s) {
            printf("[cloud] Kubernetes manifests: k8s/\n");
        }
    }

cleanup:
    cloud_config_free(cfg);
    return success;
}
