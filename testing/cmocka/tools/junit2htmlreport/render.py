#!/usr/bin/env python3
############################################################################
# apps/testing/cmocka/tools/junit2htmlreport/render.py
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
"""
Render junit reports as HTML
"""
import os

from jinja2 import Environment, FileSystemLoader, select_autoescape


class HTMLReport(object):
    def __init__(self):
        self.title = ""
        self.report = None

    def load(self, report, title="JUnit2HTML Report"):
        self.report = report
        self.title = title

    def __iter__(self):
        return self.report.__iter__()

    def __str__(self) -> str:
        current_path = os.path.dirname(os.path.abspath(__file__))
        print(current_path)
        env = Environment(
            loader=FileSystemLoader("{0}/templates".format(current_path)),
            autoescape=select_autoescape(["html"]),
        )

        template = env.get_template("report.html")
        print(template)
        return template.render(report=self, title=self.title)
