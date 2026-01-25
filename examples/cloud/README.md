# Zen-C Cloud Build System

Cloud-native build support for Zen-C with Docker and Kubernetes integration.

## Overview

The Zen-C Cloud Build system allows you to generate Docker images and Kubernetes deployment manifests directly from the compiler, using simple TOML configuration.

```
+------------------+     +----------------------+     +--------------------------+
|   app.zc         |     |   zc build           |     |   Outputs:               |
|   config.toml    | --> |   --cloud=docker|k8s | --> |   - Binary executable    |
|                  |     |   --config=file.toml |     |   - Docker image         |
+------------------+     +----------------------+     |   - Kubernetes manifests |
                                                      +--------------------------+
```

## Quick Start

### 1. Create your application

```zc
// hello.zc
fn main() -> int {
    println "Hello from Zen-C Cloud!";
    return 0;
}
```

### 2. Create configuration (optional)

```toml
# myapp.toml
[package]
name = "my-app"
version = "1.0.0"

[docker]
expose = [8080]

[kubernetes]
replicas = 3
```

### 3. Build with cloud options

```bash
# Build binary + Docker image only
zc build hello.zc -o hello --cloud=docker --config=myapp.toml

# Build binary + Docker image + Kubernetes manifests
zc build hello.zc -o hello --cloud=k8s --config=myapp.toml

# Push to registry
zc build hello.zc -o hello --cloud=docker --config=myapp.toml --push --registry=ghcr.io/youruser
```

## CLI Reference

### Cloud Build Options

| Flag | Description | Example |
|------|-------------|---------|
| `--cloud=docker` | Build Docker image only | `zc build app.zc --cloud=docker` |
| `--cloud=k8s` | Build Docker image + K8s manifests | `zc build app.zc --cloud=k8s` |
| `--config=FILE` | Specify config file | `--config=myapp.toml` |
| `--push` | Push Docker image to registry | `--push --registry=ghcr.io/user` |
| `--registry=URL` | Container registry URL | `--registry=ghcr.io/user` |
| `--tag=TAG` | Image tag (overrides config) | `--tag=v2.0.0` |

### Examples

```bash
# Docker image only
zc build myapp.zc -o myapp --cloud=docker --config=myapp.toml

# Docker + Kubernetes manifests
zc build myapp.zc -o myapp --cloud=k8s --config=myapp.toml

# With custom registry and tag
zc build myapp.zc -o myapp --cloud=docker --config=myapp.toml --registry=docker.io/myuser --tag=latest

# Build and push to registry
zc build myapp.zc -o myapp --cloud=docker --config=myapp.toml --push --registry=ghcr.io/myorg

# Verbose output for debugging
zc build myapp.zc -o myapp --cloud=k8s --config=myapp.toml --verbose
```

## Configuration: zen.toml

The TOML config file configures all aspects of cloud deployment. All sections are optional.

### Complete Reference

```toml
# ============================================================
# PACKAGE CONFIGURATION
# ============================================================
[package]
name = "my-service"              # Application name (used in image name)
version = "1.0.0"                # Version (used as default image tag)
description = "My application"   # Optional description
author = "dev@example.com"       # Optional author

# ============================================================
# DOCKER CONFIGURATION
# ============================================================
[docker]
enabled = true                   # Enable Docker build (default: true)
base_image = "debian:bookworm-slim"  # Runtime base image
builder_image = "debian:bookworm-slim"  # Build stage image
multi_stage = true               # Use multi-stage build (default: true)
workdir = "/app"                 # Working directory in container
user = "app"                     # Non-root user (set to "root" to disable)

# Exposed ports
expose = [8080, 9090]

# Environment variables
env = [
    "PORT=8080",
    "LOG_LEVEL=info",
    "DATABASE_URL=${DATABASE_URL}"
]

# Health check configuration
healthcheck_cmd = "./myapp --health"  # Health check command
healthcheck_interval = 30             # Interval in seconds
healthcheck_timeout = 5               # Timeout in seconds
healthcheck_retries = 3               # Number of retries

# Custom entrypoint (optional, defaults to binary name)
entrypoint = "./myapp --config /etc/app.conf"

# ============================================================
# KUBERNETES CONFIGURATION
# ============================================================
[kubernetes]
enabled = true                   # Enable K8s manifest generation
replicas = 3                     # Number of pod replicas
namespace = "production"         # Target namespace

# Service configuration
service_type = "ClusterIP"       # ClusterIP, NodePort, or LoadBalancer
service_port = 80                # External service port
target_port = 8080               # Container port

# Resource limits
cpu_request = "100m"             # CPU request
cpu_limit = "500m"               # CPU limit
memory_request = "128Mi"         # Memory request
memory_limit = "512Mi"           # Memory limit

# Health probes
liveness_path = "/health"        # Liveness probe path
liveness_port = 8080             # Liveness probe port
readiness_path = "/ready"        # Readiness probe path
readiness_port = 8080            # Readiness probe port

# External configuration (reference existing ConfigMaps/Secrets)
configmaps = ["app-config", "feature-flags"]
secrets = ["db-credentials", "api-keys"]

# Custom labels
labels = ["team=backend", "environment=production"]

# ============================================================
# REGISTRY CONFIGURATION
# ============================================================
[registry]
url = "ghcr.io/myorg"            # Registry URL
# tag is optional - defaults to package.version
```

### Minimal Configuration

For simple applications, a minimal config is sufficient:

```toml
[package]
name = "hello"
version = "0.1.0"

[docker]
expose = [8080]
```

Or even simpler - no config file at all! The compiler uses sensible defaults.

## Generated Outputs

### Dockerfile

The generated Dockerfile uses multi-stage builds for security and small image size:

```dockerfile
# Dockerfile generated by Zen-C Cloud Build
# Project: my-service v1.0.0

# Build stage - binary is pre-built by Zen-C compiler
FROM debian:bookworm-slim AS builder
WORKDIR /build
COPY my-service .
RUN chmod +x my-service

# Runtime stage
FROM debian:bookworm-slim

# Create non-root user
RUN groupadd -r app && useradd -r -g app app

WORKDIR /app

# Copy binary from builder
COPY --from=builder /build/my-service .
RUN chown -R app:app /app

USER app

# Expose ports
EXPOSE 8080

# Environment variables
ENV PORT=8080
ENV LOG_LEVEL=info

# Health check
HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD ./my-service --health || exit 1

# Start application
ENTRYPOINT ["./my-service"]
```

### Kubernetes Manifests

Generated in `k8s/` directory:

**k8s/deployment.yaml**
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-service
  namespace: production
  labels:
    app: my-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-service
  template:
    metadata:
      labels:
        app: my-service
    spec:
      containers:
      - name: my-service
        image: my-service:1.0.0
        imagePullPolicy: IfNotPresent
        ports:
        - containerPort: 8080
          protocol: TCP
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
        envFrom:
        - configMapRef:
            name: app-config
        - secretRef:
            name: db-credentials
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
```

**k8s/service.yaml**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
  namespace: production
  labels:
    app: my-service
spec:
  type: ClusterIP
  selector:
    app: my-service
  ports:
  - port: 80
    targetPort: 8080
    protocol: TCP
    name: http
```

## Image Pull Policy

The generated manifests use different `imagePullPolicy` values depending on your setup:

| Scenario | imagePullPolicy | When |
|----------|-----------------|------|
| Local images | `IfNotPresent` | No `--registry` flag specified |
| Remote registry | `Always` | `--registry=URL` is specified |

This allows local development with k3s/minikube without needing a registry, while ensuring production deployments always pull the latest image.

## Base Image Selection

### Default: debian:bookworm-slim

The default base image is `debian:bookworm-slim` because:
- **glibc compatibility**: Zen-C compiles to native binaries using glibc
- **Small size**: ~80MB base image
- **Security**: Regular security updates from Debian

### Alpine Linux

If you need smaller images and can compile with musl/static linking:

```toml
[docker]
base_image = "alpine:3.19"
builder_image = "alpine:3.19"
```

Note: Requires compiling with `-static` flag or musl toolchain.

### Distroless

For minimal attack surface:

```toml
[docker]
base_image = "gcr.io/distroless/base-debian12"
user = "root"  # Distroless has no shell for adduser
```

## Deployment Workflows

### Local Development with Docker

```bash
# Build and test locally
zc build app.zc -o app --cloud=docker --config=myapp.toml
docker run -p 8080:8080 my-app:0.1.0
```

### Local Kubernetes with k3s

```bash
# Generate manifests with local image
zc build app.zc -o app --cloud=k8s --config=myapp.toml

# Import image to k3s
docker save my-app:0.1.0 | sudo k3s ctr images import -

# Apply manifests
sudo kubectl apply -f k8s/

# Check status
sudo kubectl get pods
sudo kubectl logs -l app=my-app
```

### CI/CD Pipeline

```bash
# In GitHub Actions / GitLab CI
zc build app.zc -o app --cloud=k8s --config=myapp.toml --push --registry=$REGISTRY
kubectl apply -f k8s/
```

### Manual Kubernetes Deployment

```bash
# Generate manifests
zc build app.zc -o app --cloud=k8s --config=myapp.toml

# Review and apply
kubectl apply --dry-run=client -f k8s/
kubectl apply -f k8s/
```

## Project Structure

```
my-project/
  myapp.toml            # Cloud configuration
  src/
    main.zc             # Application source
  Dockerfile            # Generated by --cloud=docker
  app                   # Compiled binary
  k8s/                  # Generated by --cloud=k8s
    deployment.yaml
    service.yaml
```

## Requirements

| Tool | Required For | Installation |
|------|--------------|--------------|
| Docker | `--cloud=docker`, `--push` | [docs.docker.com](https://docs.docker.com/get-docker/) |
| kubectl | Applying K8s manifests | [kubernetes.io](https://kubernetes.io/docs/tasks/tools/) |
| k3s | Local Kubernetes | [k3s.io](https://k3s.io/) |

Check availability:
```bash
docker --version
kubectl version --client
```

## Troubleshooting

### Binary doesn't run in container

**Symptom**: `exec ./app: no such file or directory`

**Cause**: Binary compiled with glibc but running on Alpine (musl)

**Solution**: Use glibc-based image:
```toml
[docker]
base_image = "debian:bookworm-slim"
```

### Docker build fails

**Symptom**: `Cannot connect to Docker daemon`

**Solution**: Ensure Docker is running:
```bash
sudo systemctl start docker
# Or for Docker Desktop, launch the application
```

### ImagePullBackOff in k3s

**Symptom**: Pod stuck in `ImagePullBackOff` state

**Cause**: k3s can't find the image (not in registry and not imported locally)

**Solution**: Import the image to k3s:
```bash
docker save my-app:0.1.0 | sudo k3s ctr images import -
```

### kubectl apply fails

**Symptom**: `The connection to the server was refused`

**Solution**: Check cluster connection:
```bash
kubectl cluster-info
kubectl config current-context
```

### Push fails with authentication error

**Symptom**: `unauthorized: authentication required`

**Solution**: Login to your registry:
```bash
# Docker Hub
docker login

# GitHub Container Registry
echo $GITHUB_TOKEN | docker login ghcr.io -u USERNAME --password-stdin

# AWS ECR
aws ecr get-login-password | docker login --username AWS --password-stdin <account>.dkr.ecr.<region>.amazonaws.com
```

## Architecture

```
+-------------------------------------------------------------+
|                      Zen-C Compiler                          |
+-------------------------------------------------------------+
|  main.c                                                      |
|    +-- Parse CLI (--cloud=docker|k8s, --push, --registry)   |
|    +-- Compile Zen-C -> Binary                              |
|    +-- Call cloud_build() if cloud flags set                |
+-------------------------------------------------------------+
|  src/cloud/                                                  |
|    +-- toml.c         # TOML parser for config files        |
|    +-- cloud_config.c # Load and merge configuration        |
|    +-- docker.c       # Generate Dockerfile, build, push    |
|    +-- kubernetes.c   # Generate K8s manifests              |
|    +-- cloud.c        # Orchestrate cloud build pipeline    |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                    Generated Outputs                         |
+-----------------+-------------------+-----------------------+
|    Dockerfile   |  k8s/*.yaml       |   Docker Image        |
|                 |  - deployment.yaml|   (in local registry) |
|                 |  - service.yaml   |                       |
+-----------------+-------------------+-----------------------+
```

## Comparison with Other Tools

| Feature | Zen-C Cloud | Docker Compose | Kubernetes YAML | Ballerina |
|---------|-------------|----------------|-----------------|-----------|
| Single command build | Yes | No | No | Yes |
| Integrated with compiler | Yes | No | No | Yes |
| TOML config | Yes | YAML | YAML | TOML |
| Docker generation | Yes | Manual | Manual | Yes |
| K8s generation | Yes | No | Manual | Yes |
| Multi-stage builds | Yes | Yes | N/A | Yes |
| Non-root by default | Yes | No | No | Yes |

## License

Same license as Zen-C.
