// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChatglmAPI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FChatCompletionDelegate, const FString&, WordsSentence, int32, FileNameCounter, int32, CurrentCounter);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UChatglmAPI : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UChatglmAPI();

	//执行聊天主函数，委托给蓝图
	UFUNCTION(BlueprintCallable, Category = "Azure Speech")
	void PerformChatCompletion(const FString& UserContent);

	//委托聊天返回的结果（非流式用这个）
	UPROPERTY(BlueprintAssignable, Category = "Azure Speech")
	FChatCompletionDelegate OnChatCompletion;

	// 流式解析
	void ParseJson(const FString& ChunkedContent);
	//非流式解析
	void ParseJson_(const FString& ChunkedContent);


private:
	FString LastChatCompletionResult;
	FString streamCompletionResult;
};
