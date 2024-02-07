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
