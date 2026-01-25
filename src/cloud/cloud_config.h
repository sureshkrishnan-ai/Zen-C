// Zen-C Cloud Build - Configuration
// CloudConfig structure and loading from zen.toml

#ifndef ZC_CLOUD_CONFIG_H
#define ZC_CLOUD_CONFIG_H

#include "toml.h"

// Cloud configuration structure
typedef struct {
    // [package]
    char *name;
    char *version;
    char *main_file;
    char *description;
    char *author;

    // [docker]
    int docker_enabled;
    char *base_image;
    int *expose_ports;
    int expose_count;
    int multi_stage;
    char *workdir;
    char *user;
    char **env_vars;
    int env_count;
    char *healthcheck_cmd;
    int healthcheck_interval;
    int healthcheck_timeout;
    int healthcheck_retries;
    char *entrypoint;
    char *builder_image;

    // [kubernetes]
    int k8s_enabled;
    int replicas;
    char *namespace;
    char *service_type;
    int service_port;
    int target_port;
    char *cpu_request;
    char *cpu_limit;
    char *memory_request;
    char *memory_limit;
    char **labels;
    int label_count;
    char **configmaps;
    int configmap_count;
    char **secrets;
    int secret_count;
    char *liveness_path;
    int liveness_port;
    char *readiness_path;
    int readiness_port;

    // [registry]
    char *registry_url;
    char *image_tag;

} CloudConfig;

// Load configuration from zen.toml
CloudConfig *cloud_config_load(const char *toml_path);

// Create default configuration
CloudConfig *cloud_config_defaults(const char *binary_name);

// Merge CLI options into config
void cloud_config_merge_cli(CloudConfig *cfg, const char *registry_url,
                            const char *image_tag, const char *output_name);

// Free configuration
void cloud_config_free(CloudConfig *cfg);

// Get full image name (registry/name:tag)
char *cloud_config_image_name(CloudConfig *cfg);

// Debug print
void cloud_config_print(CloudConfig *cfg);

#endif // ZC_CLOUD_CONFIG_H
