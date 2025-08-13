#!/bin/bash
#
# This script stages, commits, and pushes documentation changes to GitHub.
#

echo "Adding modified documentation files..."
git add README.md TODO.md CHANGELOG.md GEMINI.md

echo "Committing files..."
git commit -m "docs: Update project status and documentation"

echo "Pushing to GitHub..."
git push

echo "Documentation sync complete."
