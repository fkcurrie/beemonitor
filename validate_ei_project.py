import requests
import json
import os

# --- Configuration ---
KEYS_FILE_PATH = 'beemonitor/edge_impulse/edge_impulse.keys'
API_BASE_URL = 'https://studio.edgeimpulse.com/v1/api/'

# --- Functions ---

def read_api_key(file_path):
    """Reads the API key from the specified file."""
    try:
        with open(file_path, 'r') as f:
            for line in f:
                if line.startswith('EI_API_KEY='):
                    return line.strip().split('=', 1)[1]
        raise ValueError("EI_API_KEY not found in the file.")
    except FileNotFoundError:
        print(f"Error: Key file not found at '{file_path}'")
        return None
    except Exception as e:
        print(f"Error reading keys file: {e}")
        return None

def validate_project_setup(api_key, project_id):
    """Connects to the Edge Impulse API and validates the project setup."""
    headers = {
        'x-api-key': api_key,
        'Accept': 'application/json',
    }
    url = f"{API_BASE_URL}{project_id}"

    print(f"Fetching project details from Edge Impulse...")
    try:
        response = requests.get(url, headers=headers)

        if response.status_code == 200:
            project_info = response.json()
            if not project_info.get('success', False):
                print(f"\n❌ FAILURE! API returned an error: {project_info.get('error', 'Unknown error')}")
                return

            # --- Validation Checks ---
            project = project_info.get('project', {})
            project_type = project.get('type')
            labeling_method = project.get('labelingMethod')

            print("\n--- Project Validation ---")
            print(f"Project Name: {project.get('name')}")
            print(f"Project Type: {project_type}")
            print(f"Labeling Method: {labeling_method}")

            if project_type == 'image' and labeling_method == 'bounding_boxes':
                print("\n✅ SUCCESS! Project is correctly configured for Image/Object Detection (FOMO).")
            else:
                print("\n❌ FAILURE! Project is not configured correctly for Object Detection.")
                print("Please ensure the project type is 'Images' and the labeling method is 'Object Detection'.")

        elif response.status_code == 401:
             print(f"\n❌ FAILURE! Received status code: {response.status_code} (Unauthorized)")
             print("Please check that your API Key is correct.")
        else:
            print(f"\n❌ FAILURE! Received status code: {response.status_code}")
            print("Response Text:", response.text)
            print("\nPlease check that your Project ID is correct.")

    except requests.exceptions.RequestException as e:
        print(f"\n❌ FAILURE! An error occurred during the request: {e}")
        print("Please check your internet connection and firewall settings.")

def main():
    """Main function to validate the project setup."""
    print(f"--- Edge Impulse Project Validator ---")

    # 1. Read API key from file
    api_key = read_api_key(KEYS_FILE_PATH)
    if not api_key:
        return

    # 2. Get Project ID from user
    project_id = input("Please enter your Edge Impulse Project ID: ")
    if not project_id.strip().isdigit():
        print("Error: Project ID must be a number.")
        return

    # 3. Validate the project
    validate_project_setup(api_key, project_id)


if __name__ == "__main__":
    main()
