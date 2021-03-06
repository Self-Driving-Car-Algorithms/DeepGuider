#!/usr/bin/env python

from active_navigation import ActiveNavigationModule
import argparse
import ov_utils.file_utils as file_utils
import numpy as np
import joblib
import os
from PIL import Image
import torch
import matplotlib.pyplot as plt


anm = ActiveNavigationModule()

# Test recovery module
# load img + action trajectory
data_list = [os.path.join('./data_exp/img_trajectory', x) for x in os.listdir('./data_exp/img_trajectory')]
data = joblib.load(np.random.choice(data_list))

img_list = data['rgb']
guidance_list = data['action']

for i in range(len(img_list)):
    anm.encodeVisualMemory(Image.fromarray(img_list[i]), guidance_list[i]) 
 
anm.enable_recovery = True
# anm.vis_mem = torch.cat(anm.vis_mem, 0)
if anm.isRecoveryGuidanceEnabled():
    curr_img = Image.fromarray(img_list[np.random.randint(len(img_list))])
    anm.calcRecoveryGuidance(img=curr_img)
    recovery_guidance = anm.recovery_guidance
    print('Recovery guidance from the last inserted visual memory : ', recovery_guidance)

# # if anm.isExplorationGuidanceEnabled():
    
# # anm.calcNeedForOptimalViewpointGuidance(topometric_pose_conf, poi_conf, entrance, entrance_conf, poi_successes)

# Test ove module
ove_data_folder = './data_exp/optimal_viewpoint/'
anm.enable_ove = True
if anm.isOptimalViewpointGuidanceEnabled():
    image_list, sf_list, bbox_list, depth_list = file_utils.get_files(ove_data_folder)
    im_paths, target_pois = file_utils.get_annos(ove_data_folder + 'anno/')
    im_cnt = 0
    tot_test = len(im_paths)
    for im_path, target_poi in zip(im_paths, target_pois):
        im_cnt += 1
        print("Testing... {}/{}, Target: {}".format(im_cnt, tot_test, target_poi))
        anm.calcOptimalViewpointGuidance(im_path, target_poi)
        guidance = anm.ov_guidance #getOptimalViewpointGuidance()
        print("Optimal Guidance [theta1, d, theta2]: [%.2f degree, %.2fm, %.2f degree]" % (guidance[0], guidance[1], guidance[2]))
anm.enable_ove = False