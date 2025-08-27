// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Http.h"
#include "QuestionAnswerRestApi.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FQuestionAnswerResponseDelegate, const FString&, Question, const FString&, Answer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestionAnswerRequestFailedDelegate, const FString&, ErrorMessage);


UCLASS(ClassGroup = (Django), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UQuestionAnswerRestApi : public UActorComponent
{
    GENERATED_BODY()

public:
    UQuestionAnswerRestApi();

    UFUNCTION(BlueprintCallable, Category = "Question Answer API")
    void GetAndDeleteEarliestAnswer(int32 ProjectId);

    UPROPERTY(BlueprintAssignable)
    FQuestionAnswerResponseDelegate OnQuestionAnswerResponse;

    UPROPERTY(BlueprintAssignable)
    FQuestionAnswerRequestFailedDelegate OnQuestionAnswerRequestFailed;

protected:
    virtual void BeginPlay() override;

private:
    void HandleRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
};