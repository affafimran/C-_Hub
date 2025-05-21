#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PolygonTriangulator.generated.h"

/**
 * 
 */
UCLASS()
class ARINDAL_LEVEL_API UPolygonTriangulator : public UObject
{
	GENERATED_BODY()


public:
	// Triangulate a ring with an optional hole
	static TArray<int32> TriangulateWithEarcut(const TArray<FVector2D>& Outer, const TArray<FVector2D>& Inner);
};
