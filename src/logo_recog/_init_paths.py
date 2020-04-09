import os
import sys

def add_path(path):
    if path not in sys.path:
        sys.path.insert(0, path)

this_dir = os.path.dirname(__file__)

# Add path to PYTHONPATH
model_path = os.path.join(this_dir, 'model')
#yolo_path = os.path.join(this_dir, 'model', 'yolo3')
add_path(model_path)
#add_path(yolo_path)
