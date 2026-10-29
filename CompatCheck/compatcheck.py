import argparse
from glob import glob
from enum import Enum
from collections import defaultdict
import json
import os

is_in_github_actions = os.getenv("GITHUB_ACTIONS") == "true"

def pluralize(string: str):
    return (string + "s" if string[-1] != "y" else string[:-1] + "ies") if string[-1] != "s" else string + "es"

class BeingInfo(Enum):
    Function = "function"
    Behavior = "behavior"
    Method = "method"
    Prop = "prop"
    Enum = "enum"
    Funcdef = "funcdef"
    Global = "global"

class DeclInfo(Enum):
    Full = "full"
    Partial = "partial"
    Deleted = "deleted"
    NoImpl = "noimpl"
    Undefined = "undefined"

class CompatInfo:
    def __init__(self):
        self.namespace = ""
        self.wanted: list[str] = []
        self.impl = ""
        self.being = ""
        self.decl: DeclInfo = DeclInfo.Undefined
        self.reason = ""
        self.of = ""
        self.games = ["TMCN", "TMU", "ESWC", "TMSX", "TMO", "TMS", "TM1"]
        self.since = ""

        self.line = 0
        self.file = ""

        self.matched = False

    def From(self, command: str, arg: str):
        match command:
            case "for":
                self.namespace = arg
            case "with":
                self.wanted.append(arg)
            case "as":
                self.impl = arg
            case "being":
                if arg not in BeingInfo:
                    raise SyntaxError(f"{arg} is not a valid type ({self.__repr__()})")
                self.being = arg
            case "of":
                self.of = arg
            case "declare":
                if arg not in DeclInfo:
                    raise SyntaxError(f"{arg} is not a valid declaration ({self.__repr__()})")
                self.decl = DeclInfo(arg)
            case "games":
                self.games = arg.split(", ")   
            case "since":
                self.since = arg         
            case "reason":
                self.reason = arg
            case _: pass

    def __repr__(self):
        return f'{{"{self.file}", line {self.line}, {self.decl.value}}}'

parser = argparse.ArgumentParser(
    prog="CompatCheck", 
    description="Creates a compatibility report for Angelscript exports", 
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)

parser.add_argument("-i", "--input", help="the directory of files to check")
parser.add_argument("-a", "--against", help="the OpenplanetCore.json file to check against")
parser.add_argument("-o", "--output", help="the output direectory of the compatibility report")
parser.add_argument("-f", "--filter", help="filter files to check", default="**/*.cpp,**/*.h")
parser.add_argument("-g", "--github", help="store result to github environ", action="store_true")
parser.add_argument("-n", "--no-fail", help="never exit with code other than 0 except when something fatal happens", action="store_true")
parser.add_argument("-r", "--recursive", action="store_true")

args = parser.parse_args()

if not args.github and is_in_github_actions:
    print("skipping post-build event")
    quit(0)

files: list[str] = []
for filter in args.filter.split(","):
    files += glob(args.input + "/" + filter, recursive=args.recursive)
    
infos: list[CompatInfo] = []
infos_by_being: dict[str, list[CompatInfo]] = defaultdict(list)
for file in files:
    with open(file, "r") as file:
        try:
            lines = file.readlines()

            tag_count = 0
            for line in lines:
                if "!CompatMeta" in line:
                    tag_count += 1

            if tag_count % 2 != 0:
                raise SyntaxError("a tag was opened but not closed")

            seen_tag = False
            for idx, line in enumerate(lines):
                if "!CompatMeta" in line:
                    seen_tag = not seen_tag
                    if not seen_tag:
                        infos[-1].line = idx + 2
                        infos[-1].file = file.name
                    else:
                        infos.append(CompatInfo())
                        infos[-1].line = idx + 2
                        infos[-1].file = file.name
                    continue
                elif seen_tag:
                    line = line.strip().removeprefix("//").strip().split(None, 1)
                    command = line[0]
                    value = line[1]

                    infos[-1].From(command, value)
        except Exception as e:
            print(f"could not parse or open {file.name}: {e}")

for info in infos:
    infos_by_being[info.being].append(info)

matches: dict[DeclInfo, int] = defaultdict(int)
misses = 0

class_name_to_idx: dict[str, int] = {}

with open(args.against, "r") as core:
    core_contents = json.load(core)
    matches_being = 0
    for i, c in enumerate(core_contents["classes"]):
        class_name_to_idx[c["name"]] = i

    for being in infos_by_being:
        is_of_class = being in ["method", "behavior", "prop"]

        if not is_of_class:
            key = pluralize(being)
            contents = core_contents[key]

            for thing in contents:
                for info in infos_by_being[being]:
                    if thing["decl"] in info.wanted:
                        matches[info.decl] += 1
                        matches_being += 1

                        info.matched = True
        else:
            for info in infos_by_being[being]:
                if info.of == "" and is_of_class:
                    raise SyntaxError(f"{info}: {info.being} of what?")
                
                contents = core_contents["classes"][class_name_to_idx[info.of]][pluralize(being)]
                for thing in contents:
                    against = thing["decl"] if being not in ["prop", "behavior"] else ((thing["typedecl"] + " " + thing["name"]) if being != "behavior" else thing["func"]["decl"])
                    if against in info.wanted:
                        matches[info.decl] += 1
                        matches_being += 1

                        info.matched = True

        misses += len(contents) - matches_being

has_unmatched_info = False
for being in infos_by_being:
    for info in infos_by_being[being]:
        if not info.matched:
            print(f"unmatched info: {info}")
            has_unmatched_info = True

complete = (matches[DeclInfo.Full] + matches[DeclInfo.Partial] + matches[DeclInfo.Deleted]) / float(misses) if misses else 0
non_partial = (matches[DeclInfo.Full] + matches[DeclInfo.Deleted]) / float(misses) if misses else 0

print(f'{"".join(f"{matches[m]} {m.value} matches, " if matches[m] != 1 else f"{matches[m]} {m.value} match, " for m in matches)}{misses} {"misses" if misses != 1 else "miss"}')
print(f'{complete:.2%} complete, {non_partial:.2%} non-partial')

if args.github:
    with open(os.environ["GITHUB_ENV"], "a") as f:
        f.write(f"COMPLETE_PERCENTAGE={complete * 100:.1f}\n")
        f.write(f"NON_PARTIAL_PERCENTAGE={non_partial * 100:.1f}\n")

if not args.no_fail: 
    quit(has_unmatched_info or (len(matches) == 0))