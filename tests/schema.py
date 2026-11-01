"""Authoring-schema checks; native profile validation remains authoritative."""
from pathlib import Path
import json
import tomllib
from jsonschema import Draft202012Validator

root = Path(__file__).resolve().parents[1]
schema = json.loads((root / "schemas/plugin.schema.json").read_text())
Draft202012Validator.check_schema(schema)
validator = Draft202012Validator(schema)
validator.validate(tomllib.loads((root / "examples/plugin.ylspec").read_text()))
for path in (root / "tests/vectors").glob("*.ylspec"):
    errors = list(validator.iter_errors(tomllib.loads(path.read_text())))
    assert bool(errors) == (path.name != "manifest.ylspec"), (path, errors)
print("JSON Schema and positive/negative profile projections passed")
