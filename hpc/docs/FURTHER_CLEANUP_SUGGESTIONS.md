# Further Cleanup Suggestions

**Current size:** 7.6GB (down from 29GB - **21.4GB recovered!**)

The cleanup exceeded expectations! Here are suggestions for further optimization if needed:

## Remaining Large Directories

### Can Potentially Remove (Save ~3.5GB)

#### 1. `results_archive_recent/` - 1.4GB
**Contents:** Result directories from 14-60 days ago
**Recommendation:** Review and delete if no longer needed
**Command:** `rm -rf results_archive_recent/`

#### 2. `original_samples_backup/` + `original_sample_surfaces_backup/` - 845MB
**Contents:** Backup copies of sample pottery data
**Recommendation:** Delete if data exists in `original_samples/` or `sfs_preprocessing/`
**Command:**
```bash
rm -rf original_samples_backup/
rm -rf original_sample_surfaces_backup/
```

#### 3. `cache/` - 813MB
**Contents:** Cached build/processing files
**Recommendation:** Safe to delete, will regenerate if needed
**Command:** `rm -rf cache/`

#### 4. `TPS_BACKUP_20250830_1419/` - 149MB
**Contents:** Old TPS backup
**Recommendation:** Delete (TPS backups already cleaned)
**Command:** `rm -rf TPS_BACKUP_20250830_1419/`

#### 5. `sfs_test_reconstruction/` - 203MB
**Contents:** Old test reconstruction files
**Recommendation:** Archive or delete if tests complete
**Check:** Review contents first
**Command:** `rm -rf sfs_test_reconstruction/`

### Should Review

#### 6. `sfs_legacy_temp/` - 1.6GB
**Contents:** Legacy SFS++ debugging copy (Tray-000 work)
**Current use:** Active debugging for Tray-000 threshold analysis
**Recommendation:** **KEEP for now** - actively used
**Future:** Can delete once Tray-000 debugging complete

#### 7. `visual_output/` - 174MB
**Contents:** Visualization outputs
**Recommendation:** Archive if no longer needed
**Check:** Review contents before deleting

## Archive Suggestions

### Compress Archives (~500MB savings)

The created archives could be further compressed:

```bash
# Optionally compress results_archive_recent/
cd results_archive_recent/
tar -czf ../results_archive_recent_20251115.tar.gz .
cd ..
rm -rf results_archive_recent/
# Saves: ~1GB disk space
```

## Aggressive Cleanup Plan

If you need maximum space savings:

```bash
# WARNING: Review before running!

# Remove all backups and caches
rm -rf original_samples_backup/
rm -rf original_sample_surfaces_backup/
rm -rf cache/
rm -rf TPS_BACKUP_20250830_1419/

# Remove test/archive data
rm -rf sfs_test_reconstruction/
rm -rf visual_output/
rm -rf results_archive_recent/

# Potential savings: ~3.5GB additional
# Final size: ~4GB
```

## Conservative Recommendation

**Safe to remove immediately:**
- `cache/` (813MB)
- `TPS_BACKUP_20250830_1419/` (149MB)
- Backup directories if duplicates confirmed

**Total safe savings: ~1GB → Final size ~6.6GB**

## Current Directory Health

The directory is now well-organized:

### Active Work (Keep)
- `sfspreproc-docker/` - Main code (73MB)
- `sfs_legacy_temp/` - Debugging (1.6GB)
- `sfspreproc.sif` - Container (815MB)
- Recent results (4 directories)

### Archives (Review periodically)
- `results_archive_old_20251115.tar.gz` (477MB)
- `logs_archive_20251115.tar.gz` (115M)
- `results_archive_recent/` (1.4GB - can compress)

### Can Remove
- Various backup directories
- Cache directories
- Old test data

## Maintenance Going Forward

**Weekly:**
- Archive results >7 days old
- Compress logs >7 days old

**Monthly:**
- Review and compress/delete results >30 days
- Delete logs >60 days
- Clean build caches

**Quarterly:**
- Review all archive tarballs
- Delete very old archives after verification
- Consolidate documentation

## Summary

- **Before:** 29GB
- **After cleanup:** 7.6GB
- **Space recovered:** 21.4GB (74% reduction!)
- **Additional potential:** ~3.5GB if aggressive cleanup
- **Minimum recommended size:** ~6-7GB (keeping active work + recent data)
