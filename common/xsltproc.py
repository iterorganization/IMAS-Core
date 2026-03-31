#!/usr/bin/env python
# simple net.sf.saxon.Transform cli replacement via saxonche Python bindings
# example invokation:
# ./xsltproc.py -xsl IDSDef2MDSpreTree.xsl -s IDSDef.xml -o output.xml DD_GIT_DESCRIBE=1 AL_GIT_DESCRIBE=1

import argparse
import logging

import saxonche


def parse_arguments() -> tuple:
    """Parse arguments, similar to net.sf.saxon.Transform..."""

    parser = argparse.ArgumentParser(
        prog="xsltproc.py",
        description="Imitates Saxon-HE's net.sf.saxon.Transform.",
        epilog="Additional arguments in format key=value will be set as xml parameters",
    )
    parser.add_argument(
        "-xsl",
        "--stylesheet_file",
        type=str,
        required=True,
        help="XSL style sheet file",
    )
    parser.add_argument(
        "-s",
        "--source_file",
        type=str,
        required=True,
        help="source XML document",
    )
    parser.add_argument(
        "-o",
        "--output_file",
        type=str,
        required=True,
        help="transformed output XML document",
    )

    args, other_args = parser.parse_known_args()
    # Convert list of strings "key=value" into dict(key=value, ...)
    other_kwargs = {k: v for k, v in map(lambda x: x.split("="), other_args)}
    return (args, other_kwargs)


def saxon_xsltproc(
    source_file: str, stylesheet_file: str, output_file: str, **kwargs
) -> None:
    with saxonche.PySaxonProcessor(license=False) as proc:
        xsltproc = proc.new_xslt30_processor()
        for key, value in kwargs.items():
            string_value = proc.make_string_value(value)
            xsltproc.set_parameter(key, string_value)
        xsltproc.transform_to_file(
            source_file=source_file,
            stylesheet_file=stylesheet_file,
            output_file=output_file,
        )


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    args, other_kwargs = parse_arguments()
    saxon_xsltproc(
        source_file=args.source_file,
        stylesheet_file=args.stylesheet_file,
        output_file=args.output_file,
        **other_kwargs,
    )
