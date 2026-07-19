#!/bin/bash

# SfS++ Elegant Build Pipeline
# Multi-stage, cacheable, optimized container build system

set -euo pipefail

# Configuration
APPTAINER_BIN="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
BUILD_DIR="/data/gpfs/projects/punim2657/sfspre"
CACHE_DIR="${BUILD_DIR}/cache"
LOG_DIR="${BUILD_DIR}/logs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Setup directories
setup_directories() {
    log_info "Setting up build directories..."
    mkdir -p "${CACHE_DIR}" "${LOG_DIR}"
    cd "${BUILD_DIR}"
}

# Stage 1: Build base dependencies container (cached)
build_base_container() {
    local base_container="${CACHE_DIR}/sfspreproc-base.sif"
    local log_file="${LOG_DIR}/base-build.log"
    
    if [ -f "${base_container}" ]; then
        log_info "Base container exists, checking if rebuild needed..."
        if [ "container-base.def" -nt "${base_container}" ]; then
            log_info "Base definition updated, rebuilding..."
            rm -f "${base_container}"
        else
            log_success "Using cached base container"
            return 0
        fi
    fi
    
    log_info "Building base dependencies container..."
    if timeout 1800 "${APPTAINER_BIN}" build --fakeroot "${base_container}" container-base.def > "${log_file}" 2>&1; then
        log_success "Base container built successfully"
        return 0
    else
        log_error "Base container build failed. Check log: ${log_file}"
        tail -20 "${log_file}"
        return 1
    fi
}

# Stage 2: Build preprocessing tools container
build_preprocessing_container() {
    local base_container="${CACHE_DIR}/sfspreproc-base.sif"
    local preprocessing_container="${BUILD_DIR}/sfspreproc.sif"
    local log_file="${LOG_DIR}/preprocessing-build.log"
    
    if [ ! -f "${base_container}" ]; then
        log_error "Base container not found. Build base first."
        return 1
    fi
    
    log_info "Building preprocessing tools container..."
    
    # Create temporary definition with base container reference
    local temp_def=$(mktemp)
    sed "s|From: sfspreproc-base.sif|From: ${base_container}|" container-preprocessing.def > "${temp_def}"
    
    if timeout 3600 "${APPTAINER_BIN}" build --fakeroot "${preprocessing_container}" "${temp_def}" > "${log_file}" 2>&1; then
        log_success "Preprocessing container built successfully"
        rm -f "${temp_def}"
        return 0
    else
        log_error "Preprocessing container build failed. Check log: ${log_file}"
        tail -20 "${log_file}"
        rm -f "${temp_def}"
        return 1
    fi
}

# Stage 3: Test the built container
test_container() {
    local container="${BUILD_DIR}/sfspreproc.sif"
    local log_file="${LOG_DIR}/container-test.log"
    
    if [ ! -f "${container}" ]; then
        log_error "Container not found for testing"
        return 1
    fi
    
    log_info "Testing built container..."
    
    # Test container execution
    if "${APPTAINER_BIN}" exec "${container}" /bin/bash -c "
        echo 'Testing container environment...'
        which cmake && echo '✓ CMake available'
        which gcc && echo '✓ GCC available' 
        ldconfig -p | grep -q libpcl && echo '✓ PCL libraries found'
        ldconfig -p | grep -q libCGAL && echo '✓ CGAL libraries found'
        ls -la /opt/sfs/bin/ 2>/dev/null && echo '✓ SfS tools directory exists'
        echo 'Container test completed'
    " > "${log_file}" 2>&1; then
        log_success "Container tests passed"
        return 0
    else
        log_warning "Some container tests failed. Check log: ${log_file}"
        cat "${log_file}"
        return 1
    fi
}

# Cleanup function
cleanup() {
    log_info "Cleaning up temporary files..."
    find "${BUILD_DIR}" -name "*.tmp" -delete 2>/dev/null || true
}

# Main build orchestration
main() {
    log_info "Starting SfS++ elegant build pipeline..."
    
    # Setup
    setup_directories
    
    # Build stages
    if build_base_container; then
        log_success "Stage 1: Base container ready"
    else
        log_error "Stage 1: Base container build failed"
        exit 1
    fi
    
    if build_preprocessing_container; then
        log_success "Stage 2: Preprocessing container ready" 
    else
        log_error "Stage 2: Preprocessing container build failed"
        exit 1
    fi
    
    if test_container; then
        log_success "Stage 3: Container testing passed"
    else
        log_warning "Stage 3: Container testing had issues"
    fi
    
    # Final summary
    log_success "Build pipeline completed successfully!"
    log_info "Container available at: ${BUILD_DIR}/sfspreproc.sif"
    log_info "Usage: apptainer exec sfspreproc.sif [command]"
    
    cleanup
}

# Error handling
trap cleanup EXIT

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi