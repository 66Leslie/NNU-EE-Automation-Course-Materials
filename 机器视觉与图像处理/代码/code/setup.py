from setuptools import setup, find_packages

setup(
    name="dam_crack_detection",
    version="1.0.0",
    author="Your Name",
    description="水下大坝裂缝检测系统",
    packages=find_packages(),
    python_requires=">=3.8",
    install_requires=[
        "Flask>=2.0.1",
        "Werkzeug>=2.0.1",
        "Pillow>=9.0.0",
        "numpy>=1.22.0",
        "opencv-python>=4.5.5.64",
        "matplotlib>=3.5.1",
        "torch>=1.10.0",
        "torchvision>=0.11.0",
        "scikit-image>=0.19.1",
        "tqdm>=4.62.3",
    ],
)
