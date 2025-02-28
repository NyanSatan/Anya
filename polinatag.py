#!/usr/bin/env python3

import os
import sys
import pwd
import socket
import json
import argparse
from enum import IntEnum
from pathlib import Path
from dataclasses import dataclass, asdict
from datetime import datetime, timezone

IGNORED_DIRS = [".git", "build"]
IGNORED_FILES = ["polinatag.py", "__tag.py"]

class AdvanceLevel(IntEnum):
    MAJOR  = 0
    MINOR  = 1
    PATCH  = 2
    QUADRO = 3
    PENTA  = 4

def error(s):
    print("[POLINATAG] " + s, file=sys.stderr)

@dataclass
class State:
    comps:      list[int]
    iteration:  int
    files:      dict

    @classmethod
    def new(cls, levels: int):
        return cls(
            [0 for _ in range(AdvanceLevel.MAJOR, levels)],
            0,
            {}
        )

@dataclass
class Database:
    name:       str
    extensions: list[str]
    levels:     int
    current:    State
    staged:     State

    @classmethod
    def new(cls, name: str, levels: int):
        if levels not in [3, 5]:
            error("only 3 and 5-component tags are allowed")
            exit(-1)

        return cls(name, [], levels, State.new(levels), None)

    @classmethod
    def deserealize(cls, inp: dict) -> "Database":
        result = cls(**inp)
        result.current = State(**result.current)
        assert len(result.current.comps) == result.levels

        if result.staged:
            result.staged = State(**result.staged)
            assert len(result.staged.comps) == result.levels

        return result
    
    def serialize(self) -> dict:
        return asdict(self)

def do_create(args):
    db_path = Path(args.database)

    if db_path.exists():
        error("this database already exists")
        exit(-1)

    db = Database.new(
        args.name,
        5 if args.pentalevel else 3
    )

    with open(db_path, "w") as f:
        f.write(
            json.dumps(db.serialize(), indent=4)
        )

    print("created the new database! Fill in file types to track in \"extensions\" array")

def _walk_root(root: Path, extensions: list[str]) -> dict:
    result = dict()

    for _root, _, files in root.walk(top_down=False):
        try:
            if _root.parts[0] in IGNORED_DIRS:
                continue
        except IndexError:
            continue

        for file in files:
            path = _root / file
            if path.name in IGNORED_FILES:
                continue

            if path.suffix in extensions:
                result[str(path)] = int(path.stat().st_mtime)

    return result

def _dict_compare(d1: dict, d2: dict) -> tuple:
    d1_keys = set(d1.keys())
    d2_keys = set(d2.keys())
    shared_keys = d1_keys.intersection(d2_keys)

    added = d1_keys - d2_keys
    removed = d2_keys - d1_keys
    modified = {o : (d1[o], d2[o]) for o in shared_keys if d1[o] != d2[o]}

    return added, removed, modified

def advance_tag(comps: list[int], factor: AdvanceLevel, levels: int) -> list[int]:
    new_comps = comps[:factor + 1]
    new_comps[-1] += 1

    for _ in range(factor + 1, levels):
        new_comps.append(0)

    return new_comps

def print_tag(name: str, comps: list[int], iteration: int, with_iter: bool = False, line_break: bool = False):
    base = name + "-"

    comps_enc = list()

    for comp in comps[::-1]:
        if comp or comps_enc:
            comps_enc.append(str(comp))

    if not comps_enc:
        error("tag is empty")
        exit(-1)

    result = base + ".".join(comps_enc[::-1])

    if with_iter:
        result += "~%d" % (iteration + 1)

    if line_break:
        endc = "\n"
    else:
        endc = ""

    print(result, end=endc)

def print_private():
    date = datetime.now(timezone.utc)
    host = "%s@%s" % (
        pwd.getpwuid(os.getuid()).pw_name,
        socket.gethostname()
    )

    result = "private_build...(%s)...%s...%s" % (
        host,
        date.strftime("%Y/%m/%d"),
        date.strftime("%H:%M:%S")
    )

    print(result, end="")

def db_open(path: Path) -> Database:
    with open(path, "r") as f:
        db_raw = json.load(f)

    return Database.deserealize(db_raw)

def do_generate(args):
    src_root = Path(args.root)
    db_path  = Path(args.database)

    try:
        db = db_open(db_path)
    except FileNotFoundError:
        print_private()
        exit(0)

    new_files = _walk_root(src_root, db.extensions)

    added, removed, modified = _dict_compare(new_files, db.current.files)

    if added or removed:
        new_comps = advance_tag(db.current.comps, AdvanceLevel.MAJOR, db.levels)
        new_iteration = 0
    elif modified:
        if len(modified) > db.levels:
            level = AdvanceLevel.MAJOR
        else:
            level = db.levels - len(modified)

        new_comps = advance_tag(db.current.comps, level, db.levels)
        new_iteration = 0
    else:
        new_comps = db.current.comps
        new_iteration = db.current.iteration + 1

    db.staged = State(new_comps, new_iteration, new_files)

    result = json.dumps(db.serialize(), indent=4)

    with open(db_path, "w") as f:
        f.write(result)

    print_tag(db.name, db.staged.comps, db.staged.iteration, with_iter=args.with_iter, line_break=False)

def do_commit(args):
    db_path = Path(args.database)

    try:
        db = db_open(db_path)
    except FileNotFoundError:
        exit(0)

    if not db.staged:
        exit(0)

    db.current = db.staged
    db.staged = None

    result = json.dumps(db.serialize(), indent=4)

    with open(db_path, "w") as f:
        f.write(result)

def do_print(args):
    db_path = Path(args.database)

    db = db_open(db_path)

    if args.staged:
        if not db.staged:
            error("nothing is staged right now")
            exit(-1)

        comps = db.staged.comps
        iteration = db.staged.iteration
    else:
        comps = db.current.comps
        iteration = db.current.iteration

    print_tag(db.name, comps, iteration, with_iter=args.with_iter, line_break=True)

def main():
    parser = argparse.ArgumentParser(description="Apple XBS-like build tag generator")
    subparsers = parser.add_subparsers()

    create_parse = subparsers.add_parser("create", help="create a new database")
    create_parse.set_defaults(func=do_create)
    create_parse.add_argument("name", help="project name")
    create_parse.add_argument("database", help="output database file")
    create_parse.add_argument("-5", dest="pentalevel", action="store_true", help="use 5-component tag (default - 3)")

    generate_parse = subparsers.add_parser("generate", help="generate build tag")
    generate_parse.set_defaults(func=do_generate)
    generate_parse.add_argument("root", help="source root")
    generate_parse.add_argument("database", help="database file")
    generate_parse.add_argument("-i", dest="with_iter", action="store_true", help="print with iteration")

    commit_parse = subparsers.add_parser("commit", help="commit build tag to database")
    commit_parse.set_defaults(func=do_commit)
    commit_parse.add_argument("database", help="database file")

    print_parse = subparsers.add_parser("print", help="just print tag")
    print_parse.set_defaults(func=do_print)
    print_parse.add_argument("database", help="database file")
    print_parse.add_argument("-s", dest="staged", action="store_true", help="print staged instead")
    print_parse.add_argument("-i", dest="with_iter", action="store_true", help="print with iteration")

    args = parser.parse_args()
    if not hasattr(args, "func"):
        parser.print_help()
        exit(-1)

    args.func(args)

if __name__ == "__main__":
    main()
