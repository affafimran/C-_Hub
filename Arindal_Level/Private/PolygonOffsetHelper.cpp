// Fill out your copyright notice in the Description page of Project Settings.


#include "PolygonOffsetHelper.h"

TArray<FVector2D> PolygonOffsetHelper::OffsetPolygonInward(const TArray<FVector2D>& OuterRing, float Inset)
{
    TArray<FVector2D> InnerRing;

    const int NumPoints = OuterRing.Num();
    if (NumPoints < 3) return InnerRing;

    for (int i = 0; i < NumPoints; ++i)
    {
        const FVector2D& Prev = OuterRing[(i - 1 + NumPoints) % NumPoints];
        const FVector2D& Curr = OuterRing[i];
        const FVector2D& Next = OuterRing[(i + 1) % NumPoints];

        // Direction vectors
        FVector2D Edge1 = (Curr - Prev).GetSafeNormal();
        FVector2D Edge2 = (Next - Curr).GetSafeNormal();

        // Perpendiculars (normals)
        FVector2D Normal1 = FVector2D(-Edge1.Y, Edge1.X);
        FVector2D Normal2 = FVector2D(-Edge2.Y, Edge2.X);

        // Bisector of the angle between Normal1 and Normal2
        FVector2D Bisector = (Normal1 + Normal2).GetSafeNormal();

        // Move inward by inset distance
        InnerRing.Add(Curr + Bisector * Inset);
    }

    return InnerRing;
}
