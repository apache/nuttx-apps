#!/usr/bin/env python3
############################################################################
# apps/testing/cmocka/tools/junit2htmlreport/parser.py
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
Parse a junit report file into a family of objects
"""
from __future__ import unicode_literals

import enum
import os
import uuid
import xml.etree.ElementTree as ET

from .render import HTMLReport
from .textutils import unicode_str


class Outcome(str, enum.Enum):
    FAILED = "failure"  # the test failed
    SKIPPED = "skipped"  # the test was skipped
    PASSED = "passed"  # the test completed successfully
    ERROR = "error"
    ABSENT = "absent"  # the test was known but not run/failed/skipped


def clean_xml_attribute(element, attribute, default=None):
    """
    Get an XML attribute value and ensure it is legal in XML
    :param element:
    :param attribute:
    :param default:
    :return:
    """

    value = element.attrib.get(attribute, default)
    if value:
        value = value.encode("utf-8", errors="replace").decode(
            "utf-8", errors="backslashreplace"
        )
        value = value.replace("\ufffd", "?")  # strip out the unicode replacement char

    return value


class ParserError(Exception):
    """
    We had a problem parsing a file
    """

    def __init__(self, message):
        super(ParserError, self).__init__(message)


class ToJunitXmlBase(object):
    """
    Base class of all objects that can be serialized to Junit XML
    """

    def tojunit(self):
        """
        Return an Element matching this object
        :return:
        """
        raise NotImplementedError()

    def make_element(self, xmltag, text=None, attribs=None):
        """
        Create an Element and put text and/or attribs into it
        :param xmltag: tag name
        :param text:
        :param attribs: dict of xml attributes
        :return:
        """
        element = ET.Element(unicode_str(xmltag))
        if text is not None:
            element.text = unicode_str(text)
        if attribs is not None:
            for item in attribs:
                element.set(unicode_str(item), unicode_str(attribs[item]))
        return element


class AnchorBase(object):
    """
    Base class that can generate a unique anchor name.
    """

    def __init__(self):
        self._anchor = None

    def id(self):
        return self.anchor()

    def anchor(self):
        """
        Generate a html anchor name
        :return:
        """
        if not self._anchor:
            self._anchor = str(uuid.uuid4())
        return self._anchor


class Property(AnchorBase, ToJunitXmlBase):
    """
    Test Properties
    """

    def __init__(self):
        super(Property, self).__init__()
        self.name = None
        self.value = None

    def tojunit(self):
        """
        Return the xml element for this property
        :return:
        """
        prop = self.make_element("property")
        prop.set("name", unicode_str(self.name))
        prop.set("value", unicode_str(self.value))
        return prop


class Case(AnchorBase, ToJunitXmlBase):
    """
    Test cases
    """

    def __init__(self):
        super(Case, self).__init__()
        self.msg = None
        self.text = None
        self.stderr = None
        self.stdout = None
        self.duration = 0
        self.name = None
        self.properties = list()
        self.outcome = Outcome.PASSED.value

    def prefix(self):
        if self.outcome == "failure":
            return "[F]"
        elif self.outcome == "skipped":
            return "[S]"
        elif self.outcome == "error":
            return "[E]"
        else:
            return ""


class Suite(AnchorBase, ToJunitXmlBase):
    """
    Contains test cases (usually only one suite per report)
    """

    def __init__(self):
        super(Suite, self).__init__()
        self.name = None
        self.duration = 0
        self.cases = list()
        self.package = None
        self.properties = list()
        self.errors = list()
        self.stdout = None
        self.stderr = None
        self.tests_num = 0
        self.failures_num = 0
        self.errors_num = 0
        self.skipped_num = 0

    def tojunit(self):
        """
        Return an element for this whole suite and all it's cases
        :return:
        """
        suite = self.make_element("testsuite")
        suite.set("name", unicode_str(self.name))
        suite.set("time", unicode_str(self.duration))
        if self.properties:
            props = self.make_element("properties")
            for prop in self.properties:
                props.append(prop.tojunit())
            suite.append(props)

        for testcase in self.all():
            suite.append(testcase.tojunit())
        return suite

    def all(self):
        """
        Return all testcases
        :return:
        """
        return self.cases

    def failed(self):
        """
        Return all the failed testcases
        :return:
        """
        return [test for test in self.all() if test.failed()]

    def skipped(self):
        """
        Return all skipped testcases
        :return:
        """
        return [test for test in self.all() if test.skipped]

    def passed(self):
        """
        Return all the passing testcases
        :return:
        """
        return [test for test in self.all() if not test.failed() and not test.skipped()]


class Junit(object):
    """
    Parse a single junit xml report
    """

    def __init__(self, filename=None, xmlstring=None):
        """
        Parse the file
        :param filename:
        :return:
        """
        self.filename = filename
        self.tree = None
        if filename is not None:
            self.tree = ET.parse(filename)
        elif xmlstring is not None:
            self._read(xmlstring)
        else:
            raise ValueError("Missing any filename or xmlstring")
        self.suites = []
        self.process()

    def __iter__(self):
        return self.suites.__iter__()

    def _read(self, xmlstring):
        """
        Populate the junit xml document tree from a string
        :param xmlstring:
        :return:
        """
        self.tree = ET.fromstring(xmlstring)

    def process(self):
        """
        populate the report from the xml
        :return:
        """
        testrun = False
        suites = None
        if isinstance(self.tree, ET.ElementTree):
            root = self.tree.getroot()
        else:
            root = self.tree

        if root.tag == "testrun":
            testrun = True
            root = root[0]

        if root.tag == "testsuite":
            suites = [root]

        if root.tag == "testsuites" or testrun:
            suites = [x for x in root]

        if suites is None:
            raise ParserError("could not find test suites in results xml")

        suitecount = 0
        for suite in suites:
            suitecount += 1
            cursuite = Suite()
            self.suites.append(cursuite)
            cursuite.name = clean_xml_attribute(
                suite, "name", default="suite-" + str(suitecount)
            )
            cursuite.package = clean_xml_attribute(suite, "package")
            cursuite.duration = float(
                suite.attrib.get("time", "0").replace(",", "") or "0"
            )
            cursuite.tests_num = int(suite.attrib.get("tests", "0"))
            cursuite.failures_num = int(suite.attrib.get("failures", "0"))
            cursuite.errors_num = int(suite.attrib.get("errors", "0"))
            cursuite.skipped_num = int(suite.attrib.get("skipped", "0"))

            for element in suite:
                if element.tag == "error":
                    # top level error?
                    errtag = {
                        "message": element.attrib.get("message", ""),
                        "type": element.attrib.get("type", ""),
                        "text": element.text,
                    }
                    cursuite.errors.append(errtag)
                if element.tag == "system-out":
                    cursuite.stdout = element.text
                if element.tag == "system-err":
                    cursuite.stderr = element.text

                if element.tag == "properties":
                    for prop in element:
                        if prop.tag == "property":
                            newproperty = Property()
                            newproperty.name = prop.attrib["name"]
                            newproperty.value = prop.attrib["value"]
                            cursuite.properties.append(newproperty)

                if element.tag == "testcase":
                    testcase = element
                    newcase = Case()
                    newcase.name = clean_xml_attribute(testcase, "name")
                    newcase.duration = float(
                        testcase.attrib.get("time", "0").replace(",", "") or "0"
                    )
                    cursuite.cases.append(newcase)

                    # does this test case have any children?
                    for child in testcase:
                        if child.tag in ["skipped", "error", "failure"]:
                            newcase.text = child.text
                            if "message" in child.attrib:
                                newcase.msg = child.attrib["message"]
                            newcase.outcome = child.tag
                        elif child.tag == "system-out":
                            newcase.stdout = child.text
                        elif child.tag == "system-err":
                            newcase.stderr = child.text
                        elif child.tag == "properties":
                            for property in child:
                                newproperty = Property()
                                newproperty.name = property.attrib["name"]
                                newproperty.value = property.attrib["value"]
                                newcase.properties.append(newproperty)

    def html(self):
        """
        Render the test suite as a HTML report with links to errors first.
        :return:
        """
        doc = HTMLReport()
        doc.load(self, os.path.basename(self.filename))
        return str(doc)
