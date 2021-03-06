/*
 * Copyright (c) 2016, The Regents of the University of California (Regents).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *    3. Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Please contact the author(s) of this library if you have any questions.
 * Authors: Erik Nelson            ( eanelson@eecs.berkeley.edu )
 */

#include <normal_passthrough/NormalPassthrough.h>
#include <parameter_utils/ParameterUtils.h>

#include <pcl/features/normal_3d.h>

namespace pu = parameter_utils;

NormalPassthrough::NormalPassthrough() {}
NormalPassthrough::~NormalPassthrough() {}

bool NormalPassthrough::Initialize(const ros::NodeHandle& n) {
  name_ = ros::names::append(n.getNamespace(), "NormalPassthrough");

  if (!LoadParameters(n)) {
    ROS_ERROR("%s: Failed to load parameters.", name_.c_str());
    return false;
  }

  if (!RegisterCallbacks(n)) {
    ROS_ERROR("%s: Failed to register callbacks.", name_.c_str());
    return false;
  }

  return true;
}

bool NormalPassthrough::LoadParameters(const ros::NodeHandle& n) {
  // Load normal computation parameters.
  if (!pu::Get("normals/search_radius", search_radius_)) return false;

  return true;
}

bool NormalPassthrough::RegisterCallbacks(const ros::NodeHandle& n) {
  // Create a local nodehandle to manage callback subscriptions.
  ros::NodeHandle nl(n);

  cloud_sub_ = nl.subscribe("point_cloud", 100,
                            &NormalPassthrough::PointCloudCallback, this);
  normal_pub_ =
      nl.advertise<NormalPointCloud>("point_cloud_with_normals", 10, false);

  return true;
}

void NormalPassthrough::PointCloudCallback(const PointCloud::ConstPtr& msg) {
  // Don't do any work if nobody is listening.
  if (normal_pub_.getNumSubscribers() == 0)
    return;

  // ComputeNormals automatically publishes the result, so we don't need the
  // returned point cloud.
  NormalPointCloud::Ptr unused(new NormalPointCloud());
  ComputeNormals(msg, unused);
}

bool NormalPassthrough::ComputeNormals(
    const PointCloud::ConstPtr& points,
    NormalPointCloud::Ptr points_with_normals) const {
  if (points_with_normals == NULL) {
    ROS_ERROR("%s: Output is null.", name_.c_str());
    return false;
  }

  // Copy point cloud header.
  points_with_normals->header = points->header;

  // Copy 3D points.
  pcl::copyPointCloud(*points, *points_with_normals);

  // Compute normals.
  pcl::NormalEstimation<pcl::PointXYZ, pcl::PointNormal> ne;
  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(
      new pcl::search::KdTree<pcl::PointXYZ>());
  ne.setSearchMethod(tree);
  ne.setInputCloud(points);
  ne.setRadiusSearch(search_radius_);
  ne.compute(*points_with_normals);

  // Publish the new point cloud.
  if (normal_pub_.getNumSubscribers() > 0) {
    normal_pub_.publish(*points_with_normals);
  }

  return true;
}
