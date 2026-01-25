// Zen-C Cloud Build - Kubernetes Manifest Generation

#ifndef ZC_KUBERNETES_H
#define ZC_KUBERNETES_H

#include "cloud_config.h"

// Generate Kubernetes manifests in output_dir
// Creates: deployment.yaml, service.yaml
// Returns 1 on success, 0 on failure
int k8s_generate_manifests(CloudConfig *cfg, const char *output_dir);

// Apply Kubernetes manifests using kubectl
// Returns 1 on success, 0 on failure
int k8s_apply_manifests(const char *manifest_dir, int verbose);

// Delete Kubernetes resources
// Returns 1 on success, 0 on failure
int k8s_delete_manifests(const char *manifest_dir, int verbose);

// Check if kubectl is available
int k8s_is_available(void);

#endif // ZC_KUBERNETES_H
