//
//  HandElementContainer.cpp
//  HandTracking
//
//  Created by Daniel Gallagher on 11/08/2016.
//  Copyright (c) 2016 Developer Co. All rights reserved.
//

#include "HandElementContainer.h"

HandElementContainer::HandElementContainer()
{
    
}

HandElementContainer::~HandElementContainer()
{
    
}

void HandElementContainer::findHandProperties(Mat binaryImage, VVPoints &outputContours,
                                              VVPoints &outputHull, VVInts &outputHullInts,
                                              VVVec4i &outputDefects, int topNContours)
{
    Mat editedImg;
    cvtColor(binaryImage, editedImg, CV_BGR2GRAY);
    
    VVPoints contours;
    VVPoints maxContours;
    vector<Vec4i> hierarchy;
    
    findContours(editedImg, contours, hierarchy, CV_RETR_TREE,
                 CV_CHAIN_APPROX_SIMPLE, Point(0,0));
    
    maxContours = getNMaxContours(contours, topNContours);
    
    VVPoints hull(maxContours.size());
    VVInts hullInts(maxContours.size());
    VVVec4i defects(maxContours.size());
    
    for (int i = 0; i < maxContours.size(); i++)
    {
        convexHull(maxContours[i], hull[i], false);
        convexHull(maxContours[i], hullInts[i], false);
        
        if (hullInts[i].size() > 3)
        {
            convexityDefects(maxContours[i], hullInts[i], defects[i]);
        }
    }
    
    outputContours = maxContours;
    outputHull = hull;
    outputHullInts = hullInts;
    outputDefects = defects;
}

VVPoints HandElementContainer::getNMaxContours(VVPoints contours, int n)
{
    VVPoints nContours;
    vector<double> contourAreas;
    
    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        contourAreas.push_back(area);
    }
    
    for (int i = 0; i < n; i++)
    {
        vector<double>::iterator result;
        
        result = max_element(contourAreas.begin(), contourAreas.end());
        
        float position = std::distance(contourAreas.begin(), result);
        
        nContours.push_back(contours[position]);
        contours.erase(contours.begin() + position);
    }
    
    return nContours;
}

Mat HandElementContainer::drawDefects(Mat frame, VVVec4i defects, VVPoints contours)
{
    Mat handConvexityDefects = Mat::zeros(frame.rows, frame.cols, CV_8UC3);
    
    for (int defIdx = 0; defIdx < contours.size(); defIdx++)
    {
        for (int i = 0; i < defects[defIdx].size(); i++)
        {
            Vec4i& v = defects[defIdx][i];
            
            float depth = v[3] / 256;
            if (depth > 10)
            {
                int startIdx = v[0];
                Point ptStart(contours[defIdx][startIdx]);
                
                int farIdx = v[1];
                Point ptFar(contours[defIdx][farIdx]);
                
                int endIdx = v[2];
                Point ptEnd(contours[defIdx][endIdx]);
                
                Scalar defectCol(0,0,255);
                
                line(handConvexityDefects, ptStart, ptEnd, Scalar(255,0,0), 1); //blue
                line(handConvexityDefects, ptStart, ptFar, Scalar(0,255,0), 1); //green
                line(handConvexityDefects, ptEnd, ptFar, Scalar(0,0,255), 1); //red
                circle(handConvexityDefects, ptFar, 4, Scalar(255,255,255), 2); //white
                
            }
        }
    }
    
    return handConvexityDefects;
}

void HandElementContainer::dismissIrrelevantDefects(VVVec4i &defects, VVPoints contours, float angleThresh)
{
    VVVec4i passedDefects;
    
    auto findSlope = [] (Point pt1, Point pt2)
    {
        float slope;
        
        int yDist = pt2.y - pt1.y;
        int xDist = pt2.x - pt1.x;
        
        // make sure we don't divide by zero
        // for the slope as makes hardware implode
        
        if (xDist == 0 || yDist == 0)
            slope = 0;
        else
            slope = yDist / xDist;
        
        return slope;
    };
    
    for (int idx = 0; idx < contours.size(); idx++)
    {
        vector<Vec4i> subDefects;
        
        for (int i = 0; i < defects[idx].size(); i++)
        {
            Vec4i defect = defects[idx][i];
            
            int startIdx = defect[0],
            farIdx = defect[1],
            endIdx = defect[2];
            
            Point ptStart(contours[idx][startIdx]),
            ptFar(contours[idx][farIdx]),
            ptEnd(contours[idx][endIdx]);
            
            float startEndSlp = findSlope(ptStart, ptEnd), //blue slope 1
            startFarSlp = findSlope(ptStart, ptFar), //green
            endFarSlp = findSlope(ptEnd, ptFar); //red slope 2
            
            float commonAngle = atan((startEndSlp + endFarSlp) / (1 - startEndSlp * endFarSlp)); //find angle between two slopes
            
            if (commonAngle <= angleThresh)
                subDefects.push_back(defect);
        }
        passedDefects.push_back(subDefects);
    }
    
    defects = passedDefects;
}
