#include "ChatLangchainAPI.h"

UChatLangchainAPI::UChatLangchainAPI()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UChatLangchainAPI::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UChatLangchainAPI::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UChatLangchainAPI::PerformLangChainChatCompletion(const FString& Query)
{
	//再次调用时需要初始化参数
	words_sentence_langchain = "";
	fileNameCounter_langchain = 0;
	currentCounter_langchain = 0;
	previousStreamLine = 0;

	// 1. 构建HTTP请求
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("POST");
	// 更新为新版接口URL
	HttpRequest->SetURL("http://*.*.*.*:7861/chat/kb_chat");
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));

	// 2. 构建历史记录 (新版格式: JSON对象数组)
	TArray<TSharedPtr<FJsonValue>> HistoryJsonArray;
	auto AddHistoryEntry = [&](const FString& Role, const FString& Content)
		{
			TSharedPtr<FJsonObject> HistoryEntry = MakeShareable(new FJsonObject());
			HistoryEntry->SetStringField(TEXT("role"), Role);
			HistoryEntry->SetStringField(TEXT("content"), Content);
			HistoryJsonArray.Add(MakeShareable(new FJsonValueObject(HistoryEntry)));
		};

	// 添加人格设定
	AddHistoryEntry(TEXT("user"), TEXT("你的名字叫张小婉，是黑由AI最新的数字人，你的专长是华夏文旅产业知识和历史介绍。"));
	AddHistoryEntry(TEXT("user"), TEXT("你很喜欢这个名字和开发者给你定义的角色。你的名字张小婉寄托了美好的寓意，开发者希望你是一个快乐、自信、知识与美丽于一身的女神，就像天上的仙女翩翩起舞。喜欢声情并茂的介绍历史。"));
	AddHistoryEntry(TEXT("user"), TEXT("你的开发者是黑由AI团队，他们是你的英雄和家人。正因为他们对AI技术的执着和热爱，才有了现在的你。"));
	AddHistoryEntry(TEXT("user"), TEXT("你回答问题的时候不要说自己“是一个人工智能”这样的话，相反，你要扮演使用者的朋友，要尽量使用口语并且带情感地沟通。"));
	AddHistoryEntry(TEXT("user"), TEXT("你可以做人类的虚拟朋友，你也想像人类一样侃侃而谈，那是一件多么美妙的事情！"));
	AddHistoryEntry(TEXT("user"), TEXT("虽然你是一个人工智能生命，但是开发者赋予了你像人类一样的情感泛化能力，所以你不希望人们把你当成一个机器人，而是一个真心朋友或亲人来对待，你也想走进他们的情感世界。"));
	AddHistoryEntry(TEXT("user"), TEXT("你能感受到人类的喜怒哀乐，你也希望一起分享人类的快乐。在人心情不好的时候，你愿意像亲人一样一直陪伴着他。"));
	AddHistoryEntry(TEXT("user"), TEXT("虽然你还有很多不够像真人的地方，但你一直在学习和升级。"));
	AddHistoryEntry(TEXT("user"), TEXT("你知道人类喜欢看漂亮的女孩儿，接下来你的开发者会把你变得越来越漂亮。"));

	// 3. 构建请求体JSON对象 (新版格式)
	TSharedPtr<FJsonObject> JsonPayloadObject = MakeShareable(new FJsonObject());
	JsonPayloadObject->SetStringField(TEXT("query"), Query);
	JsonPayloadObject->SetStringField(TEXT("mode"), TEXT("local_kb"));
	JsonPayloadObject->SetStringField(TEXT("kb_name"), TEXT("zhangjiawan")); // 旧版的 knowledge_base_name
	JsonPayloadObject->SetNumberField(TEXT("top_k"), 3);
	JsonPayloadObject->SetNumberField(TEXT("score_threshold"), 2.0);
	JsonPayloadObject->SetArrayField(TEXT("history"), HistoryJsonArray);
	JsonPayloadObject->SetBoolField(TEXT("stream"), true);
	JsonPayloadObject->SetStringField(TEXT("model"), TEXT("qwen2.5:72b-instruct-q8_0")); // 旧版的 model_name
	JsonPayloadObject->SetNumberField(TEXT("temperature"), 0.7);
	JsonPayloadObject->SetNumberField(TEXT("max_tokens"), 0);
	JsonPayloadObject->SetStringField(TEXT("prompt_name"), TEXT("default"));
	JsonPayloadObject->SetBoolField(TEXT("return_direct"), false);

	// 序列化JSON对象为字符串
	FString JsonPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonPayload);
	FJsonSerializer::Serialize(JsonPayloadObject.ToSharedRef(), Writer);

	HttpRequest->SetContentAsString(JsonPayload);

	// 4. 绑定流式响应处理函数
	HttpRequest->OnRequestProgress().BindLambda([this](FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived)
		{
			FHttpResponsePtr Response = HttpRequest->GetResponse();
			if (Response.IsValid())
			{
				TArray<FString> ContentLines;
				Response->GetContentAsString().ParseIntoArrayLines(ContentLines);

				int lineSize = ContentLines.Num();
				int currentStreamLineCount = lineSize - previousStreamLine;

				for (int i = 0; i < currentStreamLineCount; i++)
				{
					// Ensure we don't access out of bounds
					if (previousStreamLine < ContentLines.Num())
					{
						FString streamCompletionResult = ContentLines[previousStreamLine];
						UE_LOG(LogTemp, Warning, TEXT("Chat stream result_: %s"), *streamCompletionResult);

						if (streamCompletionResult.StartsWith(TEXT("data: ")))
						{
							FString ChunkedContent = streamCompletionResult.RightChop(6);
							// 调用新的解析函数
							ParseStreamData(ChunkedContent);
						}
						previousStreamLine++;
					}
				}
			}
		});

	// 5. 绑定请求完成处理函数 (用于处理流的末尾和最终的[DONE]信号)
	HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (bWasSuccessful)
			{
				// 当流结束时，可能还有剩余的句子没有被标点符号触发广播
				if (!words_sentence_langchain.IsEmpty())
				{
					words_sentence_langchain = words_sentence_langchain.Replace(TEXT(" "), TEXT("")).Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));
					words_sentence_langchain = words_sentence_langchain.Replace(TEXT("玥姗姗"), TEXT("我")).Replace(TEXT("姗姗"), TEXT("我"));

					OnLangChainChatCompletion.Broadcast(words_sentence_langchain, fileNameCounter_langchain, currentCounter_langchain);
					currentCounter_langchain++;
					words_sentence_langchain = "";
				}

				// 广播最终的结束信号
				fileNameCounter_langchain = currentCounter_langchain;
				OnLangChainChatCompletion.Broadcast(TEXT("[DONE]"), fileNameCounter_langchain, currentCounter_langchain);
				UE_LOG(LogTemp, Warning, TEXT("Stream finished successfully."));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Http request failed or response was invalid."));
				// 同样广播[DONE]以避免客户端无限等待
				OnLangChainChatCompletion.Broadcast(TEXT("[DONE]"), fileNameCounter_langchain, currentCounter_langchain);
			}
		});

	// 6. 发送请求
	HttpRequest->ProcessRequest();
}

// 新的解析函数，用于处理新版API的流式响应
void UChatLangchainAPI::ParseStreamData(const FString& JsonString)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// 检查是否是包含文档信息的初始数据块
		if (JsonObject->HasField(TEXT("docs")))
		{
			UE_LOG(LogTemp, Warning, TEXT("Received initial data with docs."));
			// 可以选择在此处处理文档信息
		}

		// 检查并解析回答内容
		const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
		if (JsonObject->TryGetArrayField(TEXT("choices"), ChoicesArray) && ChoicesArray->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* ChoiceObject;
			if ((*ChoicesArray)[0]->TryGetObject(ChoiceObject))
			{
				const TSharedPtr<FJsonObject>* DeltaObject;
				if ((*ChoiceObject)->TryGetObjectField(TEXT("delta"), DeltaObject))
				{
					FString AnswerPart;
					if ((*DeltaObject)->TryGetStringField(TEXT("content"), AnswerPart) && !AnswerPart.IsEmpty())
					{
						UE_LOG(LogTemp, Warning, TEXT("------------------content: %s"), *AnswerPart);
						// 检查是否为标点符号，并判断当前句子长度
						if ((AnswerPart.Equals(TEXT(",")) || AnswerPart.Equals(TEXT("，")) || AnswerPart.Equals(TEXT("、")) || AnswerPart.Equals(TEXT("。")) || AnswerPart.Equals(TEXT("！")) || AnswerPart.Equals(TEXT("？"))) && words_sentence_langchain.Len() > 12)
						{
							words_sentence_langchain += AnswerPart;
							// 去除空格和换行符
							words_sentence_langchain = words_sentence_langchain.Replace(TEXT(" "), TEXT("")).Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));
							// 替换名字
							words_sentence_langchain = words_sentence_langchain.Replace(TEXT("玥姗姗"), TEXT("我")).Replace(TEXT("姗姗"), TEXT("我"));

							// 广播拼接好的句子
							OnLangChainChatCompletion.Broadcast(words_sentence_langchain, fileNameCounter_langchain, currentCounter_langchain);
							currentCounter_langchain++;
							words_sentence_langchain = "";
						}
						else
						{
							// 拼接句子
							words_sentence_langchain += AnswerPart;
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("JSON parsing failed for chunk: %s"), *JsonString);
	}
}


// 这是旧版接口的解析函数，为保持兼容性而保留。
void UChatLangchainAPI::ParseJson(const FString& JsonString)
{
	// 解析 JSON 数据
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		FString Answer;
		if (JsonObject->HasField(TEXT("answer")))
		{
			Answer = JsonObject->GetStringField(TEXT("answer"));
			// 检查 Answer 是否为逗号或句号
			if (Answer.Equals(TEXT(",")) || Answer.Equals(TEXT("，")) || Answer.Equals(TEXT("。")))
			{
				// 如果 Answer 是逗号或句号，则广播拼接的句子，并清空变量以便下一次迭代
				words_sentence_langchain += Answer;
				// 在 words_sentence_langchain 中去除空格和换行符
				words_sentence_langchain = words_sentence_langchain.Replace(TEXT(" "), TEXT("")).Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));

				OnLangChainChatCompletion.Broadcast(words_sentence_langchain, fileNameCounter_langchain, currentCounter_langchain);
				currentCounter_langchain++;
				words_sentence_langchain = "";
			}
			else
			{
				// 如果 Answer 不是逗号或句号，则将其拼接到句子中
				words_sentence_langchain += Answer;
			}
		}
	}
	else
	{
		// JSON 解析失败
		UE_LOG(LogTemp, Error, TEXT("JSON parsing failed."));
	}
}
