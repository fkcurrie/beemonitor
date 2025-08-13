#!/bin/bash
#
# This script stages, commits, and pushes the Wokwi configuration files to GitHub.
#

echo "Adding Wokwi configuration files..."
git add wokwi.toml diagram.json

echo "Committing files..."
git commit -m "Add Wokwi simulation configuration"

echo "Pushing to GitHub..."
git push

echo "Sync complete. Your project should now be available on Wokwi."
