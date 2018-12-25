//
//  NSValue+NSValue_OpenCV.h
//  Synopsis-Framework
//
//  Created by vade on 3/26/17.
//  Copyright © 2017 v002. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SynopsisDenseFeature : NSObject

- (instancetype) initWithFeatureArray:(NSArray*)featureArray;

// This method concatinates a n x 1 feature with a m x 1 feaure to make a new (n + m) x 1 feature
+ (instancetype) denseFeatureByAppendingFeature:(SynopsisDenseFeature*)feature withFeature:(SynopsisDenseFeature*)feature2;
// this averages two features with the same dim, ie n x 1 and m x 1, where n = m, and where we avg each element from n with m
+ (instancetype) denseFeatureByAveragingFeature:(SynopsisDenseFeature*)feature withFeature:(SynopsisDenseFeature*)feature2;
+ (instancetype) denseFeatureByMaximizingFeature:(SynopsisDenseFeature*)feature withFeature:(SynopsisDenseFeature*)feature2;

- (NSUInteger) featureCount;

// Array like access, so one can do
// SynopsisDenseFeature* someFeature = ...
// NSNumber* zerothFeature = someFeature[0];

- (NSNumber*)objectAtIndexedSubscript:(NSUInteger)idx;
- (NSArray<NSNumber*>*) arrayValue;


@end

