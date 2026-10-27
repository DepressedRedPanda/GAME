#!/bin/bash
# scripts/verify_git_lfs.sh
# Run locally in your repository to check that Git LFS is configured and tracking Unreal asset patterns.

echo "Checking git LFS installation..."
if ! command -v git-lfs >/dev/null 2>&1; then
  echo "git-lfs not found. Install git-lfs: https://git-lfs.github.com/"
  exit 1
fi

echo "Git LFS version: $(git lfs version)"

echo "Currently tracked patterns in .gitattributes:"
cat .gitattributes

echo "
To ensure .uasset and .umap are tracked, run locally:
  git lfs install
  git lfs track \"*.uasset\" \"*.umap\" \"*.fbx\" \"*.wav\" \"*.png\" \"*.jpg\"

After generating or adding .uasset files, you can verify tracked files with:
  git lfs ls-files
" 
