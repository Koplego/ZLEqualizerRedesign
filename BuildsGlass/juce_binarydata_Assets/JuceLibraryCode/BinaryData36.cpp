/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== main.py ==================
static const unsigned char temp_binary_data_35[] =
"import os\n"
"\n"
"def modify_svg_files():\n"
"    \"\"\"\n"
"    Scans the current directory for .svg files, adds a <rect> element\n"
"    after the opening <svg> tag, and replaces the color 'black' with '#000000'.\n"
"    \"\"\"\n"
"    # Get the current working directory\n"
"    current_directory = os.getcwd()\n"
"    print(f\"Scanning for SVG files in: {current_directory}\")\n"
"\n"
"    # The line to be inserted\n"
"    rect_line = '<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\"none\" />\\n'\n"
"    \n"
"    # Iterate over all files in the current directory\n"
"    for filename in os.listdir(current_directory):\n"
"        if filename.endswith(\".svg\"):\n"
"            file_path = os.path.join(current_directory, filename)\n"
"            print(f\"Processing: {filename}\")\n"
"            \n"
"            try:\n"
"                with open(file_path, 'r', encoding='utf-8') as f:\n"
"                    lines = f.readlines()\n"
"                \n"
"                # Flags to track modifications\n"
"                made_rect_change = False\n"
"                made_color_change = False\n"
"\n"
"                # --- Modification 1: Add <rect> element ---\n"
"                svg_tag_index = -1\n"
"                for i, line in enumerate(lines):\n"
"                    if '<svg' in line:\n"
"                        svg_tag_index = i\n"
"                        break\n"
"                \n"
"                if svg_tag_index != -1:\n"
"                    # Check if the rect line already exists to avoid duplicates\n"
"                    if rect_line.strip() not in (l.strip() for l in lines):\n"
"                        lines.insert(svg_tag_index + 1, rect_line)\n"
"                        made_rect_change = True\n"
"\n"
"                # --- Modification 2: Replace 'black' with '#000000' ---\n"
"                processed_lines = []\n"
"                for line in lines:\n"
"                    new_line = line.replace('black', '#000000')\n"
"                    if new_line != line:\n"
"                        made_color_change = True\n"
"                    processed_lines.append(new_line)\n"
"\n"
"                # --- Write back to file if any changes were made ---\n"
"                if made_rect_change or made_color_change:\n"
"                    with open(file_path, 'w', encoding='utf-8') as f:\n"
"                        f.writelines(processed_lines)\n"
"                    \n"
"                    # Report what was done\n"
"                    if made_rect_change and made_color_change:\n"
"                        print(f\"  -> Added <rect> and replaced 'black' in {filename}\")\n"
"                    elif made_rect_change:\n"
"                        print(f\"  -> Successfully added <rect> element to {filename}\")\n"
"                    elif made_color_change:\n"
"                        print(f\"  -> Successfully replaced 'black' in {filename}\")\n"
"                else:\n"
"                    print(f\"  -> No changes needed for {filename}. Skipping.\")\n"
"\n"
"            except Exception as e:\n"
"                print(f\"  -> An error occurred while processing {filename}: {e}\")\n"
"\n"
"    print(\"\\nScript finished.\")\n"
"\n"
"if __name__ == \"__main__\":\n"
"    modify_svg_files()\n"
"\n";

const char* main_py = (const char*) temp_binary_data_35;
}
