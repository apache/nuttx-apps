"""
Stringify to unicode
"""


def unicode_str(text):
    """
    Convert text to unicode
    :param text:
    :return:
    """
    if isinstance(text, bytes):
        return text.decode("utf-8", "strict")
    return str(text)
