#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class ARINDAL_LEVEL_API PolygonOffsetHelper
{
public:
	// Generate an inward offset of a 2D polygon (like an inner ring)
	static TArray<FVector2D> OffsetPolygonInward(const TArray<FVector2D>& OuterRing, float Inset);
};
