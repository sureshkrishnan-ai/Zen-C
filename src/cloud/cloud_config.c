// Zen-C Cloud Build - Configuration Implementation

#include "cloud_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use real malloc for cloud module
#undef malloc
#undef free
#undef realloc
#undef calloc

static char *safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len + 1);
    }
    return dup;
}

// Load string array from TOML array
static char **load_string_array(TomlArray *arr, int *count) {
    if (!arr || arr->count == 0) {
        *count = 0;
        return NULL;
    }

    *count = arr->count;
    char **result = (char **)calloc(arr->count, sizeof(char *));

    for (int i = 0; i < arr->count; i++) {
        const char *s = toml_array_get_string(arr, i);
        result[i] = safe_strdup(s ? s : "");
    }

    return result;
}

// Load integer array from TOML array
static int *load_int_array(TomlArray *arr, int *count) {
    if (!arr || arr->count == 0) {
        *count = 0;
        return NULL;
    }

    *count = arr->count;
    int *result = (int *)calloc(arr->count, sizeof(int));

    for (int i = 0; i < arr->count; i++) {
        result[i] = (int)toml_array_get_int(arr, i);
    }

    return result;
}

CloudConfig *cloud_config_defaults(const char *binary_name) {
    CloudConfig *cfg = (CloudConfig *)calloc(1, sizeof(CloudConfig));

    // Package defaults
    cfg->name = safe_strdup(binary_name ? binary_name : "app");
    cfg->version = safe_strdup("0.1.0");
    cfg->main_file = NULL;
    cfg->description = NULL;
    cfg->author = NULL;

    // Docker defaults (use debian for glibc compatibility)
    cfg->docker_enabled = 1;
    cfg->base_image = safe_strdup("debian:bookworm-slim");
    cfg->expose_ports = NULL;
    cfg->expose_count = 0;
    cfg->multi_stage = 1;
    cfg->workdir = safe_strdup("/app");
    cfg->user = safe_strdup("app");
    cfg->env_vars = NULL;
    cfg->env_count = 0;
    cfg->healthcheck_cmd = NULL;
    cfg->healthcheck_interval = 30;
    cfg->healthcheck_timeout = 5;
    cfg->healthcheck_retries = 3;
    cfg->entrypoint = NULL;
    cfg->builder_image = safe_strdup("debian:bookworm-slim");

    // Kubernetes defaults
    cfg->k8s_enabled = 0;
    cfg->replicas = 1;
    cfg->namespace = safe_strdup("default");
    cfg->service_type = safe_strdup("ClusterIP");
    cfg->service_port = 80;
    cfg->target_port = 8080;
    cfg->cpu_request = safe_strdup("100m");
    cfg->cpu_limit = safe_strdup("500m");
    cfg->memory_request = safe_strdup("128Mi");
    cfg->memory_limit = safe_strdup("512Mi");
    cfg->labels = NULL;
    cfg->label_count = 0;
    cfg->configmaps = NULL;
    cfg->configmap_count = 0;
    cfg->secrets = NULL;
    cfg->secret_count = 0;
    cfg->liveness_path = NULL;
    cfg->liveness_port = 0;
    cfg->readiness_path = NULL;
    cfg->readiness_port = 0;

    // Registry defaults
    cfg->registry_url = NULL;
    cfg->image_tag = NULL;

    return cfg;
}

CloudConfig *cloud_config_load(const char *toml_path) {
    TomlTable *toml = toml_parse_file(toml_path);
    if (!toml) {
        return NULL;
    }

    CloudConfig *cfg = (CloudConfig *)calloc(1, sizeof(CloudConfig));

    // [package]
    cfg->name = safe_strdup(toml_get_string(toml, "package.name", "app"));
    cfg->version = safe_strdup(toml_get_string(toml, "package.version", "0.1.0"));
    cfg->main_file = safe_strdup(toml_get_string(toml, "package.main", NULL));
    cfg->description = safe_strdup(toml_get_string(toml, "package.description", NULL));
    cfg->author = safe_strdup(toml_get_string(toml, "package.author", NULL));

    // [docker]
    cfg->docker_enabled = toml_get_bool(toml, "docker.enabled", 1);
    cfg->base_image = safe_strdup(toml_get_string(toml, "docker.base_image", "debian:bookworm-slim"));
    cfg->multi_stage = toml_get_bool(toml, "docker.multi_stage", 1);
    cfg->workdir = safe_strdup(toml_get_string(toml, "docker.workdir", "/app"));
    cfg->user = safe_strdup(toml_get_string(toml, "docker.user", "app"));
    cfg->entrypoint = safe_strdup(toml_get_string(toml, "docker.entrypoint", NULL));
    cfg->builder_image = safe_strdup(toml_get_string(toml, "docker.builder_image", "debian:bookworm-slim"));

    // Docker expose ports
    TomlArray *expose_arr = toml_get_array(toml, "docker.expose");
    cfg->expose_ports = load_int_array(expose_arr, &cfg->expose_count);

    // Docker environment variables
    TomlArray *env_arr = toml_get_array(toml, "docker.env");
    cfg->env_vars = load_string_array(env_arr, &cfg->env_count);

    // Docker healthcheck
    cfg->healthcheck_cmd = safe_strdup(toml_get_string(toml, "docker.healthcheck_cmd", NULL));
    cfg->healthcheck_interval = (int)toml_get_int(toml, "docker.healthcheck_interval", 30);
    cfg->healthcheck_timeout = (int)toml_get_int(toml, "docker.healthcheck_timeout", 5);
    cfg->healthcheck_retries = (int)toml_get_int(toml, "docker.healthcheck_retries", 3);

    // [kubernetes]
    cfg->k8s_enabled = toml_get_bool(toml, "kubernetes.enabled", 0);
    cfg->replicas = (int)toml_get_int(toml, "kubernetes.replicas", 1);
    cfg->namespace = safe_strdup(toml_get_string(toml, "kubernetes.namespace", "default"));
    cfg->service_type = safe_strdup(toml_get_string(toml, "kubernetes.service_type", "ClusterIP"));
    cfg->service_port = (int)toml_get_int(toml, "kubernetes.service_port", 80);
    cfg->target_port = (int)toml_get_int(toml, "kubernetes.target_port", 8080);

    // Resources
    cfg->cpu_request = safe_strdup(toml_get_string(toml, "kubernetes.cpu_request", "100m"));
    cfg->cpu_limit = safe_strdup(toml_get_string(toml, "kubernetes.cpu_limit", "500m"));
    cfg->memory_request = safe_strdup(toml_get_string(toml, "kubernetes.memory_request", "128Mi"));
    cfg->memory_limit = safe_strdup(toml_get_string(toml, "kubernetes.memory_limit", "512Mi"));

    // Labels
    TomlArray *labels_arr = toml_get_array(toml, "kubernetes.labels");
    cfg->labels = load_string_array(labels_arr, &cfg->label_count);

    // ConfigMaps and Secrets
    TomlArray *cm_arr = toml_get_array(toml, "kubernetes.configmaps");
    cfg->configmaps = load_string_array(cm_arr, &cfg->configmap_count);

    TomlArray *sec_arr = toml_get_array(toml, "kubernetes.secrets");
    cfg->secrets = load_string_array(sec_arr, &cfg->secret_count);

    // Probes
    cfg->liveness_path = safe_strdup(toml_get_string(toml, "kubernetes.liveness_path", NULL));
    cfg->liveness_port = (int)toml_get_int(toml, "kubernetes.liveness_port", 0);
    cfg->readiness_path = safe_strdup(toml_get_string(toml, "kubernetes.readiness_path", NULL));
    cfg->readiness_port = (int)toml_get_int(toml, "kubernetes.readiness_port", 0);

    // [registry]
    cfg->registry_url = safe_strdup(toml_get_string(toml, "registry.url", NULL));
    cfg->image_tag = safe_strdup(toml_get_string(toml, "registry.tag", NULL));

    toml_free_table(toml);
    return cfg;
}

void cloud_config_merge_cli(CloudConfig *cfg, const char *registry_url,
                            const char *image_tag, const char *output_name) {
    if (!cfg) return;

    // CLI overrides TOML
    if (registry_url && strlen(registry_url) > 0) {
        free(cfg->registry_url);
        cfg->registry_url = safe_strdup(registry_url);
    }

    if (image_tag && strlen(image_tag) > 0) {
        free(cfg->image_tag);
        cfg->image_tag = safe_strdup(image_tag);
    }

    // If no name from TOML, use output name
    if (output_name && (!cfg->name || strlen(cfg->name) == 0)) {
        free(cfg->name);
        cfg->name = safe_strdup(output_name);
    }
}

void cloud_config_free(CloudConfig *cfg) {
    if (!cfg) return;

    free(cfg->name);
    free(cfg->version);
    free(cfg->main_file);
    free(cfg->description);
    free(cfg->author);

    free(cfg->base_image);
    if (cfg->expose_ports) free(cfg->expose_ports);
    free(cfg->workdir);
    free(cfg->user);
    for (int i = 0; i < cfg->env_count; i++) {
        free(cfg->env_vars[i]);
    }
    if (cfg->env_vars) free(cfg->env_vars);
    free(cfg->healthcheck_cmd);
    free(cfg->entrypoint);
    free(cfg->builder_image);

    free(cfg->namespace);
    free(cfg->service_type);
    free(cfg->cpu_request);
    free(cfg->cpu_limit);
    free(cfg->memory_request);
    free(cfg->memory_limit);
    for (int i = 0; i < cfg->label_count; i++) {
        free(cfg->labels[i]);
    }
    if (cfg->labels) free(cfg->labels);
    for (int i = 0; i < cfg->configmap_count; i++) {
        free(cfg->configmaps[i]);
    }
    if (cfg->configmaps) free(cfg->configmaps);
    for (int i = 0; i < cfg->secret_count; i++) {
        free(cfg->secrets[i]);
    }
    if (cfg->secrets) free(cfg->secrets);
    free(cfg->liveness_path);
    free(cfg->readiness_path);

    free(cfg->registry_url);
    free(cfg->image_tag);

    free(cfg);
}

char *cloud_config_image_name(CloudConfig *cfg) {
    if (!cfg) return NULL;

    char *image = (char *)malloc(512);
    const char *tag = cfg->image_tag ? cfg->image_tag : cfg->version;

    if (cfg->registry_url && strlen(cfg->registry_url) > 0) {
        snprintf(image, 512, "%s/%s:%s", cfg->registry_url, cfg->name, tag);
    } else {
        snprintf(image, 512, "%s:%s", cfg->name, tag);
    }

    return image;
}

void cloud_config_print(CloudConfig *cfg) {
    if (!cfg) return;

    printf("CloudConfig:\n");
    printf("  [package]\n");
    printf("    name: %s\n", cfg->name ? cfg->name : "(null)");
    printf("    version: %s\n", cfg->version ? cfg->version : "(null)");

    printf("  [docker]\n");
    printf("    enabled: %d\n", cfg->docker_enabled);
    printf("    base_image: %s\n", cfg->base_image ? cfg->base_image : "(null)");
    printf("    multi_stage: %d\n", cfg->multi_stage);
    printf("    expose: ");
    for (int i = 0; i < cfg->expose_count; i++) {
        printf("%d ", cfg->expose_ports[i]);
    }
    printf("\n");

    printf("  [kubernetes]\n");
    printf("    enabled: %d\n", cfg->k8s_enabled);
    printf("    replicas: %d\n", cfg->replicas);
    printf("    namespace: %s\n", cfg->namespace ? cfg->namespace : "(null)");
    printf("    service_type: %s\n", cfg->service_type ? cfg->service_type : "(null)");

    printf("  [registry]\n");
    char *full_image = cloud_config_image_name(cfg);
    printf("    full_image: %s\n", full_image ? full_image : "(null)");
    free(full_image);
}
