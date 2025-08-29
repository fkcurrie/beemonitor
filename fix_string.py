import re

# Read the original C++ file
with open('/home/fcurrie/Projects/esp32s2/src/main.cpp', 'r') as f:
    content = f.read()

# This is the broken C++ code block that we need to find and replace.
# We use a regular expression to find it reliably.
# This pattern looks for 'String body = "<h2>Administer</h2>" + message +'
# and matches everything (including newlines) until the first semicolon.
pattern = re.compile(r"String body = \"<h2>Administer</h2>\" \+ message \+[^\s\S]*?;", re.MULTILINE)

# This is the corrected C++ code block.
# Note the corrected string concatenation and the double backslash in the regex (\\d).
new_code_block = r'''    String body = "<h2>Administer</h2>" + message +
                  "<div style='display: flex; gap: 2rem;">" +
                  "<div class='card' style='flex: 1;'>" +
                  "<h3>Change Password</h3>" +
                  "<form action='/admin/changepass' method='POST'>" +
                  "<div class='form-group'><label>Current Password</label><input type='password' name='current_password' required></div>" +
                  "<div class='form-group'><label>New Password</label><input type='password' name='new_password' pattern='(?=.*\\d)(?=.*[a-z])(?=.*[A-Z])(?=.*[^A-Za-z0-9]).{8,}' title='Must contain at least one number, one uppercase, one lowercase, one special character, and at least 8 or more characters' required></div>" +
                  "<div class='form-group'><label>Confirm New Password</label><input type='password' name='confirm_password' required></div>" +
                  "<button type='submit'>Update Password</button>" +
                  "</form>" +
                  "</div>" +
                  "<div class='card' style='flex: 1;'>" +
                  "<h3>System</h3>" +
                  "<p>The currently set timezone is: <strong>" + currentTz + "</strong></p>" +
                  "<form action='/admin/find-timezone' method='POST' style='margin-bottom:1rem;'>" +
                  "<button type='submit'>Auto-Detect Timezone</button>" +
                  "</form>" +
                  "<form action='/admin/reboot' method='POST' onsubmit='return confirm(\"Are you sure you want to reboot?\" );' style='margin-bottom:1rem;'>" +
                  "<button type='submit'>Reboot Device</button>" +
                  "</form>" +
                  "<form action='/factory-reset' method='POST' onsubmit='return confirm(\"Are you sure? This erases all settings.\" );'>" +
                  "<button type='submit' class='danger'>Factory Reset</button>" +
                  "</form>" +
                  "</div>" +
                  "</div>";'''

# Perform the replacement
updated_content, count = re.subn(pattern, new_code_block, content)

if count > 0:
    print(f"Successfully found and replaced the code block.")
    # Write the corrected content back to the file
    with open('/home/fcurrie/Projects/esp32s2/src/main.cpp', 'w') as f:
        f.write(updated_content)
else:
    print("Error: Could not find the code block to replace.")
    # Let's see what the content looks like around the area of interest
    try:
        start_index = content.index('void handleAdminPage')
        print("DEBUG: Content around handleAdminPage:")
        print(content[start_index:start_index+1000])
    except ValueError:
        print("DEBUG: 'void handleAdminPage' not found in the file.")
