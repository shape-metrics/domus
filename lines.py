import os

lines_per_cpp = {}
lines_per_header = {}

def count_lines(root_dir, blacklist):
    for root, dirs, files in os.walk(root_dir):
        dirs[:] = [d for d in dirs if d not in blacklist]
        
        for file in files:
            if file in blacklist:
                continue
            if not file.endswith(".cpp") and not file.endswith(".hpp"):
                continue
                
            file_path = os.path.join(root, file)
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    if file.endswith(".cpp"):
                        lines_per_cpp[file_path] = sum(1 for _ in f)
                    if file.endswith(".hpp"):
                        lines_per_header[file_path] = sum(1 for _ in f)
            except Exception as e:
                print(f"Could not read {file_path}: {e}")

if __name__ == "__main__":
    target_directories = ['src', 'include']
    ignored_items = {
        'kissat', 'glucose', 'nlohmann'
    }
    
    for target_directory in target_directories:
        count_lines(target_directory, ignored_items)

    lines_per_cpp = dict(sorted(lines_per_cpp.items(), key=lambda item: item[1]))
    lines_per_header = dict(sorted(lines_per_header.items(), key=lambda item: item[1]))

    for file in lines_per_header:
        print(f"{lines_per_header[file]:>8} | {file}")
    print(f"total header lines: {sum(lines_per_header.values())}")
    for file in lines_per_cpp:
        print(f"{lines_per_cpp[file]:>8} | {file}")
    print(f"total cpp lines: {sum(lines_per_cpp.values())}")

    print(f"total lines: {sum(lines_per_header.values()) + sum(lines_per_cpp.values())}")