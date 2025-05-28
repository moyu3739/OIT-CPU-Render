# 统计指定目录下指定文件类型的代码行数

import os
from rich.console import Console
from rich.table import Table

# 需要统计的目录和文件类型
DIR_LIST = [
    "src",
    "src/kernel",
    "src/usecase",
    "src/test",
]
TYPE_LIST = [".h", ".cpp"]


def GetLoc(content: str):
    return len(content.split('\n'))

def GetFileLoc(filename: str):
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            return GetLoc(f.read())
    except:
        return "Not a text file"
    
def SearchDirectory(dir: str, filetype_list: list[str]):
    """
    在目录 `dir` 递归查找指定文件类型的文件，并统计各文件行数和总行数
    """
    file_record = {}
    dir_loc = 0
    for root, dirs, files in os.walk(dir):
        for file in files:
            filename = os.path.join(root, file)
            if os.path.splitext(filename)[1] in filetype_list or ".*" in filetype_list:
                loc = GetFileLoc(filename)
                relative_path = os.path.relpath(filename, dir)
                file_record[relative_path] = loc
                if isinstance(loc, int): dir_loc += loc
    return file_record, dir_loc


def main():
    console = Console()
    dir_record = {} # 记录每个目录下的代码行数
    total_loc = 0 # 总代码行数

    # 遍历目标目录，统计各目录下的代码行数，每个目标目录输出一张表
    for dir in DIR_LIST:
        file_record, dir_loc = SearchDirectory(dir, TYPE_LIST)
        dir_record[dir] = dir_loc
        total_loc += dir_loc

        table = Table()
        table.add_column(dir, justify="left", style="green")
        table.add_column("LOC", justify="right", style="cyan")
        for file, loc_file in file_record.items():
            table.add_row(file, str(loc_file))
        table.add_row("[ SUM ]", str(dir_loc), style="bold magenta1")
        console.print(table)
        print()

    # 输出总代码行数，输出一张表，每个目标目录一行
    table = Table()
    table.add_column("DIR", justify="left", style="magenta1")
    table.add_column("LOC", justify="right", style="cyan")
    for dir, loc_dir in dir_record.items():
        table.add_row(dir, str(loc_dir))
    table.add_row("[ SUM ]", str(total_loc), style="bold orange1")
    console.print(table)


if __name__ == '__main__':
    print("\033c", end="", flush=True)  # 清空终端
    main()
    input("\nPress Enter to exit...")
