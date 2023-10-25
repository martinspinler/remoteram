from setuptools import Extension, setup

setup(
    name = "remoteram",
    version = "0.1.0",
    author = "Martin Spinler",
    author_email = "spinler@cesnet.cz",
    #py_modules=['nfb'],
    packages=['remoteram'],
    #package_dir={'nfb': 'nfb'},
    package_data = {
        'remoteram': ['protobuf/*'],
    },
)
