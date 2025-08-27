// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Http.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "ChatLangchainAPI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLangChainChatCompletionDelegate, const FString&, Answer, int32, FileNameCounter, int32, CurrentCounter);

UCLASS(ClassGroup = (LangChain), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UChatLangchainAPI : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UChatLangchainAPI();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Function to perform chat completion
    UFUNCTION(BlueprintCallable, Category = "Langchain")
    void PerformLangChainChatCompletion(const FString& Query);

    // Event delegate to handle chat completion
    UPROPERTY(BlueprintAssignable, Category = "Langchain")
    FLangChainChatCompletionDelegate OnLangChainChatCompletion;

private:
    // New function to parse the streaming data from the new API
    void ParseStreamData(const FString& JsonString);

    // Original function, now unused but kept to avoid breaking changes.
    void ParseJson(const FString& JsonString);

    // Member variables to manage state for a single request
    FString words_sentence_langchain;
    int32 fileNameCounter_langchain;
    int32 currentCounter_langchain;
    int32 previousStreamLine;



};
