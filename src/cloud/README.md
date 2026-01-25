# Zen-C Cloud Build Module

Internal documentation for the cloud-native build system implementation.

## Module Structure

```
src/cloud/
├── cloud.h           # Main module header
├── cloud.c           # Build orchestration entry point
├── toml.h            # TOML parser header
├── toml.c            # Lightweight TOML parser implementation
├── cloud_config.h    # CloudConfig structure definition
├── cloud_config.c    # Configuration loading and merging
├── docker.h          # Docker generation header
├── docker.c          # Dockerfile generation and docker commands
├── kubernetes.h      # Kubernetes generation header
├── kubernetes.c      # K8s manifest generation
└── README.md         # This file
```

## Key Data Structures

### CloudConfig (cloud_config.h)

Central configuration structure holding all cloud deployment settings:

```c
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
    // ... more fields

    // [kubernetes]
    int k8s_enabled;
    int replicas;
    char *namespace;
    char *service_type;
    // ... more fields

    // [registry]
    char *registry_url;
    char *image_tag;
} CloudConfig;
```

### TOML Parser (toml.h)

Simple TOML parser supporting:
- Basic types: string, int, bool
- Arrays
- Tables (sections)
- Dotted keys (e.g., `kubernetes.resources.cpu`)
- Inline tables

```c
TomlTable *toml_parse_file(const char *path);
const char *toml_get_string(TomlTable *t, const char *path, const char *default_val);
long long toml_get_int(TomlTable *t, const char *path, long long default_val);
TomlArray *toml_get_array(TomlTable *t, const char *path);
```

## Entry Point

The main entry point is `cloud_build()` in cloud.c:

```c
int cloud_build(const char *binary_path, const char *binary_name,
                int mode_docker, int mode_k8s, int mode_push, int mode_deploy,
                const char *registry_url, const char *image_tag, int verbose);
```

Called from `main.c` after successful binary compilation when cloud flags are set.

## Build Flow

```
1. main.c parses CLI flags (--docker, --k8s, etc.)
2. Zen-C compilation produces binary
3. If cloud flags set, call cloud_build()
4. cloud_build():
   a. Load zen.toml via toml_parse_file()
   b. Create CloudConfig from TOML (or defaults)
   c. Merge CLI options (registry, tag override TOML)
   d. If --docker: generate Dockerfile, run docker build
   e. If --k8s: generate k8s/*.yaml files
   f. If --push: run docker push
   g. If deploy: run kubectl apply
```

## Memory Management

The cloud module uses real malloc/free (not the arena allocator) because:
1. It runs after compilation is complete
2. Configuration structures have complex lifetimes
3. Simpler to manage with standard allocation

```c
// At top of each .c file
#undef malloc
#undef free
#undef realloc
#undef calloc
```

## Adding New Features

### Adding a new zen.toml field

1. Add field to `CloudConfig` struct in `cloud_config.h`
2. Add loading logic in `cloud_config_load()` in `cloud_config.c`
3. Add default value in `cloud_config_defaults()`
4. Add cleanup in `cloud_config_free()`
5. Use the field in `docker.c` or `kubernetes.c`

### Adding a new CLI flag

1. Add field to `CompilerConfig` in `src/zprep.h`
2. Add parsing in `main.c` argument loop
3. Pass to `cloud_build()` or access via `g_config`
4. Update `print_usage()` help text

### Adding new K8s resource type

1. Add generator function in `kubernetes.c`:
   ```c
   static int generate_ingress(CloudConfig *cfg, const char *output_dir);
   ```
2. Call from `k8s_generate_manifests()`
3. Add to kustomization.yaml resources list

## Testing

```bash
# Build compiler
make

# Test Docker generation
cd examples/cloud
../../zc build hello.zc -o hello --docker --verbose

# Test K8s generation
../../zc build hello.zc -o hello --k8s --verbose

# Verify Docker image runs
docker run --rm hello-cloud:0.1.0

# Validate K8s manifests
kubectl apply --dry-run=client -f k8s/
```

## Dependencies

- Standard C library (stdio, stdlib, string, sys/stat)
- No external TOML libraries (custom lightweight parser)
- External tools: docker, kubectl (invoked via system())

## Error Handling

Functions return int (1 = success, 0 = failure) and print errors to stderr:

```c
if (!docker_is_available()) {
    fprintf(stderr, "[cloud] Error: Docker is not available\n");
    return 0;
}
```

## Future Enhancements

- [ ] Helm chart generation
- [ ] Docker Compose generation
- [ ] Ingress resource generation
- [ ] HPA (Horizontal Pod Autoscaler)
- [ ] Network policies
- [ ] Service accounts
- [ ] Pod disruption budgets
- [ ] Support for multiple containers (sidecars)
