#!/usr/bin/env python3
############################################################################
# apps/testing/cmocka/tools/cmocka_report.py
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

import json
import os
import xml.etree.ElementTree as ET
from enum import Enum

import typer
import xmltodict
from bs4 import BeautifulSoup
from junit2htmlreport.parser import Junit


class ConvertType(str, Enum):
    XML2JSON = "xml2json"
    XML2HTML = "xml2html"
    MERGEXML = "merge"


class CmockaReport:
    def __init__(self, xml, out):
        self.xml = xml
        self.out = out

    def xml2dict(self):
        """Parse the XML file and convert it into a dictionary"""
        try:
            with open(self.xml, "r") as f:
                content = f.read()
                soup = BeautifulSoup(content, "xml")
                xml_dict = xmltodict.parse(str(soup))
                return xml_dict
        except FileNotFoundError:
            print("No such file or directory: {0}".format(self.xml))
        except Exception:
            print("Failed to parse XML file")

    def xml2json(self):
        """Convert XML dictionary into a JSON string"""
        xml_dict = self.xml2dict()
        if xml_dict is None:
            return

        json_data = json.dumps(xml_dict, indent=4)
        if self.out:
            try:
                f = open(self.out, "w")
                f.write(json_data)
                f.close()
                print("Job Done")
            except FileNotFoundError:
                print("No such file or directory: {0}".format(self.out))
            except Exception:
                print("Failed to write json file")
        else:
            print(json_data)

    def xml2html(self):
        """Convert XML file into a html file"""
        if not self.out:
            self.out = "{0}.html".format(self.xml.split(".")[0])
        try:
            report = Junit(self.xml)
            html = report.html()
            f = open(self.out, "wb")
            f.write(html.encode("UTF-8"))
            f.close()
            print("Job Done")
        except FileNotFoundError:
            print("No such file: {0}".format(self.out))
        except Exception:
            print("Failed to write html file")

    def mergexml(self):
        """Merge multiple XML files into one"""
        merged = ET.Element("testsuites")
        for _ in os.listdir(self.xml):
            if _.endswith(".xml"):
                try:
                    tree = ET.parse(os.path.join(self.xml, _))
                    root = tree.getroot()
                    merged.extend(list(root))
                except ET.ParseError as e:
                    print("Error parsing XML:", _, e)
                    return
        if not merged:
            print("Can not find any xml file")
            return
        if self.out:
            try:
                ET.ElementTree(merged).write(
                    self.out, encoding="UTF-8", xml_declaration=True
                )
                print("Job Done")
            except FileNotFoundError:
                print("No such file or directory: {0}".format(self.out))
            except Exception:
                print("Failed to write merge xml file")
        else:
            ET.dump(merged)


app = typer.Typer()


@app.command()
def main(
    operate: ConvertType = typer.Option(
        default=ConvertType.XML2JSON, help="operation type"
    ),
    xml: str = typer.Option(default=None, help="where is the xml file or xml dir"),
    out: str = typer.Option(default=None, help="write to output instead of stdout"),
):
    """
    :param operate: operation type\n
    :param xml: where where xml file\n
    :param out: write to output instead of stdout\n
    """
    if xml is None:
        raise typer.BadParameter("Please provide xml file or xml dir")
    rpt = CmockaReport(xml, out)
    if operate == ConvertType.XML2JSON:
        rpt.xml2json()
    elif operate == ConvertType.XML2HTML:
        rpt.xml2html()
    elif operate == ConvertType.MERGEXML:
        rpt.mergexml()
    else:
        pass


if __name__ == "__main__":
    app()
