import os
import csv
import re
import subprocess
from typing import Type
from pydantic import BaseModel, Field
import pandas as pd
from openai import OpenAI
from FileCache import FileCache

# -------------------------------
# OpenAI Client Setup
# -------------------------------
client = OpenAI(
    api_key="your-key", # Replace with your actual API key
    base_url="https://openrouter.ai/api/v1",
)
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ghp
# -------------------------------
# Environment Variable Setup
# -------------------------------
# Define directories from your DevEco Studio installation
node_home = "/Users/cem/Desktop/DevEco-Studio.app/Contents/tools/node"
hvigorw_dir = "/Users/cem/Desktop/DevEco-Studio.app/Contents/tools/hvigor/bin"
java_home = "/Users/cem/Desktop/DevEco-Studio.app/Contents/jbr/Contents/Home"
sdk_home = "/Users/cem/Desktop/DevEco-Studio.app/Contents/sdk"

# Set environment variables to mirror DevEco Studio's environment
os.environ["NODE_HOME"] = node_home
os.environ["DEVECO_SDK_HOME"] = sdk_home
os.environ["JAVA_HOME"] = java_home

# Update PATH to include necessary binaries
node_bin = os.path.join(node_home, "bin")
java_bin = os.path.join(java_home, "bin")
os.environ["PATH"] = hvigorw_dir + os.pathsep + node_bin + os.pathsep + java_bin + os.pathsep + os.environ.get("PATH", "")

# -------------------------------
# File Paths and Caching
# -------------------------------
index_file_path = "/Users/cem/DevEcoStudioProjects/pycharm_project/entry/src/main/ets/pages/Index.ets"

# Initialize file cache to manage the Index.ets file
file_cache = FileCache()
file_cache.cache_file(index_file_path)

# -------------------------------
# Core Functions
# -------------------------------

def send_prompt_to_openai_router(prompt):
    """Sends a prompt to the LLM and returns the response."""
    response = client.chat.completions.create(
        model="deepseek/deepseek-chat-v3-0324",
        messages=[
            {"role": "system", "content": "You are a UI developer who uses arkTS language which is a new programming language extended from typescript. You should only write arkTS code"},
            {"role": "user", "content": [{"type": "text", "text": prompt}]}
        ]
    )
    answer = response.choices[0].message.content.strip()
    return answer

def compile_file():
    """Runs the ArkTS compiler via the hvigor wrapper and returns the result."""
    env = os.environ.copy()
    node_path = os.path.join(node_bin, "node")
    hvigor_path = os.path.join(hvigorw_dir, "hvigorw.js")

    build_cmd = [
        node_path, hvigor_path,
        "--mode", "module", "-p", "module=entry@default", "-p", "product=default",
        "-p", "requiredDeviceType=phone", "-p", "arkts.compiler.ets.tsType.nullable=true",
        "-p", "arkts.compiler.ets.type-check=false", "assembleHap",
        "--analyze=normal", "--parallel", "--incremental=false", "--daemon"
    ]

    try:
        result = subprocess.run(build_cmd, check=True, capture_output=True, text=True, env=env)
        return 0, result.stdout
    except subprocess.CalledProcessError as e:
        return e.returncode, e.stderr

def get_llm_response(prompt):
    """Gets a response from a fine-tuned LLM."""
    completion = client.chat.completions.create(
        model="ft:gpt-4o-mini-2024-07-18:personal:arktsfinetunedb32:BI8BVdIe",
        messages=[
            {"role": "system", "content": "You are a UI developer who uses arkTS language which is a new programming language extended from typescript."},
            {"role": "user", "content": prompt}
        ]
    )
    return completion.choices[0].message.content

def extract_code_from_response(response):
    """Extracts ArkTS code blocks from the LLM response markdown."""
    code_pattern = r"```(?:arkts|typescript|ts|javascript|js)?\s*([\s\S]*?)\s*```"
    matches = re.findall(code_pattern, response)
    if matches:
        return max(matches, key=len).strip()
    else:
        return response.strip()

def enhance_error_with_source_line(error_message):
    """Enhances compiler error messages with the relevant source code line and context."""
    file_line_pattern = r'File: (.*?):(\d+):(\d+)'
    match = re.search(file_line_pattern, error_message)

    if not match:
        return error_message

    file_path, line_number, column = match.groups()
    line_number, column = int(line_number), int(column)

    if not os.path.isfile(file_path):
        return error_message

    try:
        with open(file_path, 'r') as f:
            lines = f.readlines()

        if 0 < line_number <= len(lines):
            source_line = lines[line_number - 1].rstrip()
            pointer = ' ' * (column - 1) + '^'
            enhanced_error = f"{error_message}\n\nSource code at {file_path}:{line_number}:{column}:\n{source_line}\n{pointer}"

            context_start = max(0, line_number - 4)
            context_end = min(len(lines), line_number + 3)
            context = []
            for i in range(context_start, context_end):
                prefix = '> ' if (i + 1) == line_number else '  '
                context.append(f"{prefix}{i + 1}: {lines[i].rstrip()}")
            enhanced_error += "\n\nContext:\n" + "\n".join(context)
            return enhanced_error
        else:
            return error_message
    except Exception:
        return error_message

def count_errors(message):
    """Counts the number of 'ERROR' occurrences in a compiler message."""
    return message.upper().count('ERROR') if message else 0

# -------------------------------
# ArkTS Compiler Tool Definition
# -------------------------------

class ArkTSCompilerInput(BaseModel):
    """Input for the ArkTS compiler, expecting the full source code of an .ets file."""
    index_code: str = Field(..., description="The full ArkTS source code for the Index.ets file.")

class ArkTSCompiler:
    """
    A tool that compiles ArkTS code within a HarmonyOS project by updating
    the Index.ets file and running the standard hvigor build command.
    """
    name: str = "ArkTS Code Compiler"
    description: str = (
        "This tool validates ArkTS code by placing it into an existing HarmonyOS project and running the build process. "
        "If the build succeeds, it returns a code of 0. If it fails, it returns the compiler's error message."
    )
    args_schema: Type[BaseModel] = ArkTSCompilerInput

    def _run(self, argument: ArkTSCompilerInput) -> tuple[int, str]:
        """Takes ArkTS code, writes it to the Index.ets file, compiles, and returns the result."""
        file_cache.update_file(index_file_path, new_content=argument.index_code)
        return_code, message = compile_file()
        return return_code, message

# -------------------------------
# Evaluation and Utility Functions
# -------------------------------

def create_evaluation_csv(filename="evaluationb16.csv"):
    """Creates a new CSV file for evaluation with predefined headers and instructions."""
    headers = ["Test Instructions", "Fine-tuned Model Output", "Pass1 Error Count", "Pass2 Error Count", "Pass3 Error Count"]
    test_instructions = [
        "Create an object in the screen and enable the user to drag it somewhere else. (The code should be implemented using arkTS language)",
        "Implement a toggle component which acts as a on and off switch (The code should be implemented using arkTS language)",
        "Implement an array with fruit names and show them in a list in the screen. (The code should be implemented using arkTS language)",
        "Implement an array with fruit names and for each fruit put its image next to the current item in the screen. You may assume that al the names of the images are “app.media.icon”. (The code should be implemented using arkTS language)",
        "Implement a long array of list of items and make sure that the list is scrollable.  (The code should be implemented using arkTS language)",
        "Create a slider component from 0 to 100. (The code should be implemented using arkTS language)",
        "Show a yellow circle in the center of the screen and write a text underneath saying “shape circle” The code should be implemented using arkTS language)",
    ]
    rows = [[instruction, "", "", "", ""] for instruction in test_instructions]
    with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(headers)
        writer.writerows(rows)
    print(f"CSV file '{filename}' has been created successfully.")

def evaluate_llm_performance(csv_filename="evaluationb16.csv"):
    """Runs a multi-pass evaluation of an LLM using instructions from a CSV file."""
    try:
        df = pd.read_csv(csv_filename)
    except FileNotFoundError:
        print(f"Error: File {csv_filename} not found.")
        return

    _compilerTool = ArkTSCompiler()

    for index, row in df.iterrows():
        instruction = row["Test Instructions"]
        print(f"Processing instruction {index + 1}/{len(df)}: {instruction}")

        # PASS 1
        llm_response = get_llm_response(instruction)
        current_code = extract_code_from_response(llm_response)
        return_code, error_message = _compilerTool._run(ArkTSCompilerInput(index_code=current_code))
        df.at[index, "Pass1 Error Count"] = count_errors(error_message) if return_code != 0 else 0

        # PASS 2
        if return_code != 0:
            enhanced_error = enhance_error_with_source_line(error_message)
            pass2_prompt = f"{instruction}\n\nHere's my current code with compilation errors:\n\n```\n{current_code}\n```\n\nCompilation errors:\n{enhanced_error}\n\nPlease fix the code."
            llm_response = get_llm_response(pass2_prompt)
            current_code = extract_code_from_response(llm_response)
            return_code, error_message = _compilerTool._run(ArkTSCompilerInput(index_code=current_code))
            df.at[index, "Pass2 Error Count"] = count_errors(error_message) if return_code != 0 else 0
        else:
            df.at[index, "Pass2 Error Count"] = 0

        # PASS 3
        if return_code != 0:
            enhanced_error = enhance_error_with_source_line(error_message)
            pass3_prompt = f"{instruction}\n\nMy code still has compilation errors:\n\n```\n{current_code}\n```\n\nCompilation errors:\n{enhanced_error}\n\nPlease provide a completely fixed version."
            llm_response = get_llm_response(pass3_prompt)
            current_code = extract_code_from_response(llm_response)
            return_code, error_message = _compilerTool._run(ArkTSCompilerInput(index_code=current_code))
            df.at[index, "Pass3 Error Count"] = count_errors(error_message) if return_code != 0 else 0
        else:
            df.at[index, "Pass3 Error Count"] = 0

        df.at[index, df.columns[1]] = current_code
        df.to_csv(csv_filename, index=False)
        print(f"Completed instruction {index + 1}/{len(df)}.")


# -------------------------------
# Main Execution Block
# -------------------------------
if __name__ == "__main__":
    # Example of a single instruction test
    instruction = (
        "Show a yellow circle in the center of the screen and write a text underneath "
        "saying 'shape circle'. The code should be implemented using arkTS language."
    )

    print("--- Running Single Instruction Test ---")
    llm_response = send_prompt_to_openai_router(instruction)
    code = extract_code_from_response(llm_response)
    print("\nGenerated Code:\n", code)

    # Use the refactored compiler tool
    compiler_tool = ArkTSCompiler()
    return_code, message = compiler_tool._run(ArkTSCompilerInput(index_code=code))

    print("\n--- Compilation Result ---")
    print(f"Return Code: {return_code}")
    if return_code == 0:
        print("Build successful!")
    else:
        print("\nBuild failed with error:\n", enhance_error_with_source_line(message))

