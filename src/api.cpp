﻿#include <json/json.h>
#include <tgbot/bot.h>
#include <tgbot/methods/api.h>
#include <tgbot/utils/encode.h>
#include <tgbot/utils/https.h>

#define unused __attribute__((__unused__))
#define BOOL_TOSTR(xvalue) ((xvalue) ? "true" : "false")
#define SEPARATE(k, sstr) \
  if (k) sstr << ','

using namespace tgbot::methods;
using namespace tgbot::utils;

template<typename _Ty>
using Ptr = ::tgbot::types::Ptr<_Ty>;

static void inline __attribute__((__always_inline__))
parseJsonObject(const std::string &serialized, Json::Value &v) {
	Ptr<Json::CharReader> parser{Json::CharReaderBuilder().newCharReader()};

	const char *bodyBeginPointer{serialized.c_str()};
	const char *bodyEndPointer{bodyBeginPointer + serialized.size()};

	unused std::string reportedErrors;

	parser->parse(bodyBeginPointer, bodyEndPointer, &v, &reportedErrors);
}

static inline std::string toString(
		const types::ShippingOption &shippingOption) {
	std::stringstream jsonify;
	jsonify << "{ \"id\": \"" << shippingOption.id << "\", \"title\": \""
	        << shippingOption.title << "\""
	        << ", \"prices\": [";

	const size_t &nPrices = shippingOption.prices.size();
	for (size_t i = 0; i < nPrices; i++) {
		SEPARATE(i, jsonify);

		const types::LabeledPrice &price = shippingOption.prices.at(i);
		jsonify << "{ \"amount\":" << price.amount << ", \"label\":\""
		        << price.label << "\" }";
	}

	jsonify << "]}";

	return jsonify.str();
}

static inline void allowedUpdatesToString(
		const std::vector<api_types::UpdateType> &updates,
		std::stringstream &stream) {
	stream << "[";
	for (auto const &update : updates) {
		switch (update) {
			case api_types::UpdateType::MESSAGE:
				stream << "\"message\",";
				break;
			case api_types::UpdateType::CALLBACK_QUERY:
				stream << "\"callback_query\",";
				break;
			case api_types::UpdateType::INLINE_QUERY:
				stream << "\"inline_query\",";
				break;
			case api_types::UpdateType::CHOSEN_INLINE_RESULT:
				stream << "\"chosen_inline_result\",";
				break;
			case api_types::UpdateType::PRE_CHECKOUT_QUERY:
				stream << "\"pre_checkout_query\",";
				break;
			case api_types::UpdateType::SHIPPING_QUERY:
				stream << "\"shipping_query\",";
				break;
			case api_types::UpdateType::EDITED_CHANNEL_POST:
				stream << "\"edited_channel_post\",";
				break;
			case api_types::UpdateType::EDITED_MESSAGE:
				stream << "\"edited_message\",";
				break;
			case api_types::UpdateType::CHANNEL_POST:
				stream << "\"channel_post\",";
				break;
		}
	}
}

static inline void removeComma(const std::stringstream &stream,
                               std::string &target) {
	std::string &&req = stream.str();
	char &endpos = req.at(req.size() - 1);
	if (endpos == ',') endpos = ']';
	target = req;
}

static inline std::string toString(
		const api_types::MaskPosition &maskPosition) {
	std::stringstream jsonify;
	jsonify << "{ \"point\": \"" << maskPosition.point
	        << "\",\"x_shift\": " << maskPosition.xShift
	        << ",\"y_shift\": " << maskPosition.yShift
	        << ",\"scale\": " << maskPosition.scale;

	return jsonify.str();
}

static void invoiceParams(std::stringstream &stream,
                          const types::Invoice &params) {
	stream << "&title=";
	encode(stream, params.title);

	stream << "&description=";
	encode(stream, params.description);

	stream << "&currency=" << params.currency;

	stream << "&provider_token=";
	encode(stream, params.providerToken);

	stream << "&start_parameter=";
	encode(stream, params.startParameter);

	stream << "&payload=";
	encode(stream, params.payload);

	if (params.sendEmailToProvider) stream << "&send_email_to_provider=true";

	if (params.sendPhoneNumberToProvider)
		stream << "&send_phone_number_to_provider=true";

	if (params.isFlexible) stream << "&is_flexible=true";

	if (params.needEmail) stream << "&need_email=true";

	if (params.needName) stream << "&need_name=true";

	if (params.needPhoneNumber) stream << "&need_phone_number=true";

	if (params.needShippingAddress) stream << "&need_shipping_address=true";

	if (params.photoHeight) stream << "&photo_height=" << params.photoHeight;

	if (params.photoSize) stream << "&photo_size=" << params.photoSize;

	if (params.photoWidth) stream << "&photo_width=" << params.photoWidth;

	if (params.photoUrl) stream << "&photo_url=" << *params.photoUrl;

	if (params.providerData) {
		stream << "&provider_data";
		encode(stream, *params.providerData);
	}

	stream << "&prices=%5B";

	std::stringstream pricesStream;

	for (std::size_t i = 0; i < params.prices.size(); ++i) {
		SEPARATE(i, pricesStream);
		pricesStream << "{\"label\":" << params.prices.at(i).label
		             << ",\"amount\":" << params.prices.at(i).amount << "}";
	}

	encode(stream, pricesStream.str());
	stream << "%5D";
}

static std::string arrayOfInputMediaSerializer(http::PostForms &forms,
                                               std::vector<Ptr<types::InputMedia>> const &media) {
	std::stringstream mediaSerializedStream;

	mediaSerializedStream << "[";
	for (size_t i = 0; i < media.size(); ++i) {
		SEPARATE(i, mediaSerializedStream);
		mediaSerializedStream << media[i]->toString();
		if (media[i]->fileSource == tgbot::methods::types::FileSource::LOCAL_UPLOAD) {
			const char *_media{media[i]->media.c_str()};
			forms[_media] = http::value{_media, nullptr, nullptr};
		}
	}

	mediaSerializedStream << "]";
	return mediaSerializedStream.str();
}

// Api constructors

// Webhook, no further action
tgbot::methods::Api::Api(const std::string &token)
		: baseApi("https://api.telegram.org/bot" + token) {}

// Webhook
tgbot::methods::Api::Api(
		const std::string &token, const std::string &url, const int &maxConnections,
		const std::vector<api_types::UpdateType> &allowedUpdates)
		: baseApi("https://api.telegram.org/bot" + token) {
	if (!setWebhook(url, maxConnections, allowedUpdates))
		throw TelegramException("Unable to set webhook");
}

// Webhook with cert
tgbot::methods::Api::Api(
		const std::string &token, const std::string &url,
		const std::string &certificate, const int &maxConnections,
		const std::vector<api_types::UpdateType> &allowedUpdates)
		: baseApi("https://api.telegram.org/bot" + token) {
	if (!setWebhook(url, certificate, maxConnections, allowedUpdates))
		throw TelegramException("Unable to set webhook");
}

// HTTP Long polling
tgbot::methods::Api::Api(
		const std::string &token,
		const std::vector<api_types::UpdateType> &allowedUpdates,
		const int &timeout, const int &limit)
		: baseApi("https://api.telegram.org/bot" + token), currentOffset(0) {
	std::stringstream fullApiRequest;
	fullApiRequest << baseApi << "/getUpdates?limit=" << limit
	               << "&timeout=" << timeout;

	if (!allowedUpdates.empty()) {
		fullApiRequest << "&allowed_updates=";
		allowedUpdatesToString(allowedUpdates, fullApiRequest);
		removeComma(fullApiRequest, updateApiRequest);
	} else
		updateApiRequest = fullApiRequest.str();
}

//
// API implementations
//

// getUpdates (internal usage)
int tgbot::methods::Api::getUpdates(void *c,
                                    std::vector<api_types::Update> &updates) {
	std::stringstream updatesRequest;
	updatesRequest << updateApiRequest << "&offset=" << currentOffset;

	Json::Value rootUpdate;
	parseJsonObject(utils::http::get(c, updatesRequest.str()), rootUpdate);

	try {
		if (!rootUpdate.get("ok", "").asBool()) {
			const std::string description{
					rootUpdate.get("description", "").asCString()};
			logger.error(description);
			throw TelegramException{description};
		}
	} catch (const Json::LogicError &e) {
		return 0;
	}

	Json::Value valueUpdates = rootUpdate.get("result", "");
	const int &updatesCount = valueUpdates.size();
	if (!updatesCount) return 0;

	for (auto const &singleUpdate : valueUpdates)
		updates.emplace_back(singleUpdate);

	currentOffset =
			1 + valueUpdates[updatesCount - 1].get("update_id", "").asInt();

	return updatesCount;
}

//
// Availible API Methods
//

// setWebhook
bool tgbot::methods::Api::setWebhook(
		const std::string &url, const int &maxConnections,
		const std::vector<api_types::UpdateType> &allowedUpdates) {
	std::stringstream request;
	request << baseApi << "/setWebhook?url=" << url
	        << "&max_connections=" << maxConnections;

	std::string setWebhookRequest;
	if (!allowedUpdates.empty()) {
		request << "&allowed_updates=";
		allowedUpdatesToString(allowedUpdates, request);
		removeComma(request, setWebhookRequest);
	} else
		setWebhookRequest = request.str();

	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(http::get(inst, setWebhookRequest), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	urlWebhook = url;
	return true;
}

bool tgbot::methods::Api::setWebhook(
		const std::string &url, const std::string &certificate,
		const int &maxConnections,
		const std::vector<api_types::UpdateType> &allowedUpdates) {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	http::PostForms forms;
	forms["certificate"] = http::value{nullptr, certificate.c_str(), nullptr};
	forms["url"] = http::value{url.c_str(), nullptr, nullptr};
	forms["max_connections"] = http::value{std::to_string(maxConnections).c_str(), nullptr, nullptr};

	if (allowedUpdates.empty()) {
		parseJsonObject(http::multiPartUpload(inst, baseApi + "/setWebhook", forms), value);
	} else {
		std::stringstream request;
		std::string final;

		allowedUpdatesToString(allowedUpdates, request);
		removeComma(request, final);

		forms["allowed_updates"] = http::value{final.c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/setWebhook", forms),
				value);
	}

	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	urlWebhook = url;
	return true;
}

// deleteWebhook
bool tgbot::methods::Api::deleteWebhook() const {
	CURL *inst = http::curlEasyInit();
	bool isOk =
			(http::get(inst, baseApi + "/deleteWebhook").find("\"ok\":true") !=
			 std::string::npos);
	curl_easy_cleanup(inst);

	if (!isOk) throw TelegramException("Cannot delete webhook");

	return true;
}

// getWebhookInfo
api_types::WebhookInfo tgbot::methods::Api::getWebhookInfo() const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(http::get(inst, baseApi + "/getWebhookInfo"), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::WebhookInfo(value.get("result", ""));
}

// getMe
api_types::User tgbot::methods::Api::getMe() const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(http::get(inst, baseApi + "/getMe"), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::User(value.get("result", ""));
}

// getChat
api_types::Chat tgbot::methods::Api::getChat(const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(http::get(inst, baseApi + "/getChat?chat_id=" + chatId),
	                value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Chat(value.get("result", ""));
}

// getChatMembersCount
unsigned tgbot::methods::Api::getChatMembersCount(
		const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(
			http::get(inst, baseApi + "/getChatMembersCount?chat_id=" + chatId),
			value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return value.get("result", "").asUInt();
}

// getFile
api_types::File tgbot::methods::Api::getFile(const std::string &fileId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(http::get(inst, baseApi + "/getFile?file_id=" + fileId),
	                value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::File(value.get("result", ""));
}

// getChatMember
api_types::ChatMember tgbot::methods::Api::getChatMember(
		const std::string &chatId, const int &userId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/getChatMember?chat_id=" << chatId
	    << "&user_id=" << userId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::ChatMember(value.get("result", ""));
}

// getStickerSet
api_types::StickerSet tgbot::methods::Api::getStickerSet(
		const std::string &name) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(
			http::get(inst, baseApi + "/getStickerSet?name=" + encode(name)), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::StickerSet(value.get("result", ""));
}

// getUserProfilePhotos
api_types::UserProfilePhotos tgbot::methods::Api::getUserProfilePhotos(
		const int &userId, const unsigned int &offset,
		const unsigned int &limit) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/getUserProfilePhotos?user_id=" << userId
	    << "&offset=" << offset << "&limit=" << limit;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::UserProfilePhotos(value.get("result", ""));
}

// getChatAdministrators
std::vector<api_types::ChatMember> tgbot::methods::Api::getChatAdministrators(
		const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(
			http::get(inst, baseApi + "/getChatAdministrators?chat_id=" + chatId),
			value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	std::vector<api_types::ChatMember> members;
	for (auto const& member : value.get("result", ""))
		members.emplace_back(member);

	return members;
}

// getGameHighScores
std::vector<api_types::GameHighScore> tgbot::methods::Api::getGameHighScores(
		const int &userId, const int &chatId, const int &messageId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/getGameHighScores?user_id=" << userId
	    << "&chat_id=" << chatId << "&message_id=" << messageId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	std::vector<api_types::GameHighScore> members;
	for (auto const& member : value.get("result", ""))
		members.emplace_back(member);

	return members;
}

std::vector<api_types::GameHighScore> tgbot::methods::Api::getGameHighScores(
		const int &userId, const std::string &inlineMessageId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/getGameHighScores?user_id=" << userId
	    << "&inline_message_id=" << inlineMessageId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	std::vector<api_types::GameHighScore> members;
	for (auto const& member : value.get("result", ""))
		members.emplace_back(member);

	return members;
}

// deleteChatPhoto
bool tgbot::methods::Api::deleteChatPhoto(const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(
			http::get(inst, baseApi + "/deleteChatPhoto?chat_id=" + chatId), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// deleteMessage
bool tgbot::methods::Api::deleteMessage(const std::string &chatId,
                                        const std::string &messageId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/deleteMessage?chat_id=" << chatId
	    << "&message_id=" << messageId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// deleteStickerFromSet
bool tgbot::methods::Api::deleteStickerFromSet(
		const std::string &sticker) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(
			http::get(inst, baseApi + "/deleteStickerFromSet?sticker=" + sticker),
			value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// exportChatInviteLink
std::string tgbot::methods::Api::exportChatInviteLink(
		const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(
			http::get(inst, baseApi + "/exportChatInviteLink?chat_id=" + chatId),
			value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return value.get("result", "").asCString();
}

// kickChatMember
bool tgbot::methods::Api::kickChatMember(const std::string &chatId,
                                         const int &userId,
                                         const int &untilDate) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/kickChatMember?chat_id=" << chatId
	    << "&user_id=" << userId;

	if (untilDate != -1) url << "&until_date=" << untilDate;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// leaveChat
bool tgbot::methods::Api::leaveChat(const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	parseJsonObject(http::get(inst, baseApi + "/leaveChat?chat_id=" + chatId),
	                value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// pinChatMessage
bool tgbot::methods::Api::pinChatMessage(
		const std::string &chatId, const std::string &messageId,
		const bool &disableNotification) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/pinChatMessage?chat_id=" << chatId
	    << "&message_id=" << messageId;

	if (disableNotification) url << "&disable_notification=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// promoteChatMember
bool tgbot::methods::Api::promoteChatMember(
		const std::string &chatId, const int &userId,
		const types::ChatMemberPromote &permissions) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/promoteChatMember?chat_id=" << chatId
	    << "&user_id=" << userId
	    << "&can_post_messages=" << BOOL_TOSTR(permissions.canPostMessages)
	    << "&can_change_info=" << BOOL_TOSTR(permissions.canChangeInfo)
	    << "&can_edit_messages=" << BOOL_TOSTR(permissions.canEditMessages)
	    << "&can_delete_messages=" << BOOL_TOSTR(permissions.canDeleteMessages)
	    << "&can_invite_users=" << BOOL_TOSTR(permissions.canInviteUsers)
	    << "&can_restrict_members=" << BOOL_TOSTR(permissions.canRestrictMembers)
	    << "&can_pin_messages=" << BOOL_TOSTR(permissions.canPinMessages)
	    << "&can_promote_members=" << BOOL_TOSTR(permissions.canPromoteMembers);

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// restrictChatMember
bool tgbot::methods::Api::restrictChatMember(
		const std::string &chatId, const int &userId,
		const types::ChatMemberRestrict &permissions, const int &untilDate) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/restrictChatMember?chat_id=" << chatId
	    << "&user_id=" << userId
	    << "&can_send_messages=" << BOOL_TOSTR(permissions.canSendMessages)
	    << "&can_send_media_messages="
	    << BOOL_TOSTR(permissions.canSendMediaMessages)
	    << "&can_send_other_messages="
	    << BOOL_TOSTR(permissions.canSendOtherMessages)
	    << "&can_add_web_page_previews="
	    << BOOL_TOSTR(permissions.canAddWebPagePreviews);

	if (untilDate != -1) url << "&until_date=" << untilDate;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// unbanChatMember
bool tgbot::methods::Api::unbanChatMember(const std::string &chatId,
                                          const int &userId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/unbanChatMember?chat_id=" << chatId
	    << "&user_id=" << userId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// unpinChatMessage
bool tgbot::methods::Api::unpinChatMessage(const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/unpinChatMessage?chat_id=" << chatId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// setChatDescription
bool tgbot::methods::Api::setChatDescription(
		const std::string &chatId, const std::string &description) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/setChatDescription?chat_id=" << chatId << "&description=";
	encode(url, description);

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// setChatTitle
bool tgbot::methods::Api::setChatTitle(const std::string &chatId,
                                       const std::string &title) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/setChatTitle?chat_id=" << chatId << "&title=";
	encode(url, title);

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// setChatPhoto
bool tgbot::methods::Api::setChatPhoto(const std::string &chatId,
                                       const std::string &filename,
                                       const std::string &mimeType) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	http::PostForms forms;
	forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
	forms["photo"] = http::value{nullptr, filename.c_str(), mimeType.c_str()};

	parseJsonObject(http::multiPartUpload(inst, baseApi + "/setChatPhoto", forms),
	                value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// setGameScore
api_types::Message tgbot::methods::Api::setGameScore(
		const std::string &userId, const int &score, const int &chatId,
		const int &messageId, const bool &force,
		const bool &disableEditMessage) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/setGameScore?user_id=" << userId << "&score=" << score
	    << "&chat_id=" << chatId << "&message_id=" << messageId;

	if (force) url << "&force=true";

	if (disableEditMessage) url << "&disable_edit_message=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::setGameScore(
		const std::string &userId, const int &score,
		const std::string &inlineMessageId, const bool &force,
		const bool &disableEditMessage) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/setGameScore?user_id=" << userId << "&score=" << score
	    << "&inline_message_id=" << inlineMessageId;

	if (force) url << "&force=true";

	if (disableEditMessage) url << "&disable_edit_message=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// setStickerPositionInSet
bool tgbot::methods::Api::setStickerPositionInSet(const std::string &sticker,
                                                  const int &position) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/setStickerPositionInSet?sticker=" << sticker
	    << "&position=" << position;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// uploadStickerFile
api_types::File tgbot::methods::Api::uploadStickerFile(
		const int &userId, const std::string &pngSticker,
		const types::FileSource &source) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/uploadStickerFile?user_id=" << userId
		    << "&png_sticker=" << pngSticker;

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["user_id"] = http::value{std::to_string(userId).c_str(), nullptr, nullptr};
		forms["png_sticker"] = http::value{nullptr, pngSticker.c_str(), "image/png"};
		parseJsonObject(http::multiPartUpload(inst, baseApi + "/uploadStickerFile", forms),
		                value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::File(value.get("result", ""));
}

// addStickerToSet
bool tgbot::methods::Api::addStickerToSet(
		const int &userId, const std::string &name, const std::string &emoji,
		const std::string &pngSticker, const types::FileSource &source) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/addStickerToSet?user_id=" << userId
		    << "&png_sticker=" << pngSticker << "&name=";
		encode(url, name);
		url << "&emoji=" << emoji;

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["user_id"] = http::value{std::to_string(userId).c_str(), nullptr, nullptr};
		forms["name"] = http::value{name.c_str(), nullptr, nullptr};
		forms["emoji"] = http::value{emoji.c_str(), nullptr, nullptr};
		forms["png_sticker"] = http::value{nullptr, pngSticker.c_str(), "image/png"};
		parseJsonObject(http::multiPartUpload(inst, baseApi + "/addStickerToSet", forms),
		                value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

bool tgbot::methods::Api::addStickerToSet(
		const int &userId, const std::string &name, const std::string &emoji,
		const std::string &pngSticker, const api_types::MaskPosition &maskPosition,
		const types::FileSource &source) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	const std::string &&serMaskPosition = toString(maskPosition);

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/addStickerToSet?user_id=" << userId
		    << "&png_sticker=" << pngSticker << "&name=";
		encode(url, name);
		url << "&emoji=" << emoji << "&mask_position=" << serMaskPosition;

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["user_id"] = http::value{std::to_string(userId).c_str(), nullptr, nullptr};
		forms["name"] = http::value{name.c_str(), nullptr, nullptr};
		forms["emoji"] = http::value{emoji.c_str(), nullptr, nullptr};
		forms["png_sticker"] = http::value{nullptr, pngSticker.c_str(), "image/png"};
		forms["mask_position"] = http::value{serMaskPosition.c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/addStickerToSet", forms),
				value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// createNewStickerSet
bool tgbot::methods::Api::createNewStickerSet(
		const int &userId, const std::string &name, const std::string &title,
		const std::string &emoji, const std::string &pngSticker,
		const types::FileSource &source) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/createNewStickerSet?user_id=" << userId
		    << "&png_sticker=" << pngSticker << "&name=";
		encode(url, name);
		url << "&emoji=" << emoji << "&title=" << title;

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["user_id"] = http::value{std::to_string(userId).c_str(), nullptr, nullptr};
		forms["name"] = http::value{name.c_str(), nullptr, nullptr};
		forms["emoji"] = http::value{emoji.c_str(), nullptr, nullptr};
		forms["png_sticker"] = http::value{nullptr, pngSticker.c_str(), "image/png"};
		forms["title"] = http::value{title.c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/addStickerToSet", forms),
				value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

bool tgbot::methods::Api::createNewStickerSet(
		const int &userId, const std::string &name, const std::string &title,
		const std::string &emoji, const std::string &pngSticker,
		const api_types::MaskPosition &maskPosition,
		const types::FileSource &source) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	const std::string &&serMaskPosition = toString(maskPosition);

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/createNewStickerSet?user_id=" << userId
		    << "&png_sticker=" << pngSticker << "&name=";
		encode(url, name);
		url << "&emoji=" << emoji << "&title=" << title
		    << "&mask_position=" << serMaskPosition;

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["user_id"] = http::value{std::to_string(userId).c_str(), nullptr, nullptr};
		forms["name"] = http::value{name.c_str(), nullptr, nullptr};
		forms["emoji"] = http::value{emoji.c_str(), nullptr, nullptr};
		forms["png_sticker"] = http::value{nullptr, pngSticker.c_str(), "image/png"};
		forms["title"] = http::value{title.c_str(), nullptr, nullptr};
		forms["mask_position"] = http::value{serMaskPosition.c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/addStickerToSet", forms),
				value);
	}

	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// answerPreCheckoutQuery
bool tgbot::methods::Api::answerPreCheckoutQuery(
		const std::string &preCheckoutQueryId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi
	    << "/answerPreCheckoutQuery?pre_checkout_query_id=" << preCheckoutQueryId
	    << "&ok=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

bool tgbot::methods::Api::answerPreCheckoutQuery(
		const std::string &preCheckoutQueryId,
		const std::string &errorMessage) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi
	    << "/answerPreCheckoutQuery?pre_checkout_query_id=" << preCheckoutQueryId
	    << "&ok=false"
	    << "&error_message=" << errorMessage;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// answerShippingQuery
bool tgbot::methods::Api::answerShippingQuery(
		const std::string &shippingQueryId, const std::string &errorMessage) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/answerShippingQuery?shipping_query_id=" << shippingQueryId
	    << "&ok=false"
	    << "&error_message=";
	encode(url, errorMessage);

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

bool tgbot::methods::Api::answerShippingQuery(
		const std::string &shippingQueryId,
		const std::vector<types::ShippingOption> &shippingOptions) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/answerShippingQuery?shipping_query_id=" << shippingQueryId
	    << "&ok=true&shipping_options=%5B";

	std::stringstream optionsStream;
	const size_t &nOptions = shippingOptions.size();
	for (size_t i = 0; i < nOptions; i++) {
		SEPARATE(i, optionsStream);
		optionsStream << toString(shippingOptions.at(i));
	}

	encode(url, optionsStream.str());
	url << "%5D";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// answerCallbackQuery
bool tgbot::methods::Api::answerCallbackQuery(
		const std::string &callbackQueryId, const std::string &text,
		const bool &showAlert, const std::string &url, const int &cacheTime) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream surl;
	surl << baseApi
	     << "/answerCallbackQuery?callback_query_id=" << callbackQueryId;

	if (!text.empty()) {
		surl << "&text=";
		encode(surl, text);
	}

	if (showAlert) surl << "&show_alert=true";

	if (!url.empty()) surl << "&url=" << url;

	if (cacheTime) surl << "&cache_time=" << cacheTime;

	parseJsonObject(http::get(inst, surl.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// answerInlineQuery
bool tgbot::methods::Api::answerInlineQuery(
		const std::string &inlineQueryId,
		const std::vector<Ptr<types::InlineQueryResult>> &results,
		const int &cacheTime, const bool &isPersonal, const std::string &nextOffset,
		const std::string &switchPmText,
		const std::string &switchPmParameter) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/answerInlineQuery?inline_query_id=" << inlineQueryId
	    << "&results=%5B";

	std::stringstream resultsStream;
	const size_t &nResults = results.size();
	for (size_t i = 0; i < nResults; i++) {
		SEPARATE(i, resultsStream);
		resultsStream << results.at(i)->toString();
	}

	encode(url, resultsStream.str());
	url << "%5D";

	if (cacheTime) url << "&cache_time=" << cacheTime;

	if (isPersonal) url << "&is_personal=true";

	if (!nextOffset.empty()) url << "&next_offset=" << nextOffset;

	if (!switchPmText.empty()) {
		url << "&switch_pm_text=";
		encode(url, switchPmText);
	}

	if (!switchPmParameter.empty()) {
		url << "&switch_pm_parameter=";
		encode(url, switchPmParameter);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// sendMessage
api_types::Message tgbot::methods::Api::sendMessage(
		const std::string &chatId, const std::string &text,
		const types::ParseMode &parseMode, const bool &disableWebPagePreview,
		const bool &disableNotification,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendMessage?chat_id=" << chatId << "&text=";
	encode(url, text);

	if (parseMode == types::ParseMode::HTML)
		url << "&parse_mode=HTML";
	else if (parseMode == types::ParseMode::MARKDOWN)
		url << "&parse_mode=Markdown";

	if (disableWebPagePreview) url << "&disable_web_page_preview=true";

	if (disableNotification) url << "&disable_notificatiton=true";

	const std::string &&markup = replyMarkup.toString();
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::sendMessage(
		const std::string &chatId, const std::string &text,
		const int &replyToMessageId, const types::ParseMode &parseMode,
		const bool &disableWebPagePreview, const bool &disableNotification,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendMessage?chat_id=" << chatId << "&text=";
	encode(url, text);
	url << "&reply_to_message_id=" << replyToMessageId;

	if (parseMode == types::ParseMode::HTML)
		url << "&parse_mode=HTML";
	else if (parseMode == types::ParseMode::MARKDOWN)
		url << "&parse_mode=Markdown";

	if (disableWebPagePreview) url << "&disable_web_page_preview=true";

	if (disableNotification) url << "&disable_notificatiton=true";

	const std::string &&markup = replyMarkup.toString();
	if (markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// forwardMessage
api_types::Message tgbot::methods::Api::forwardMessage(
		const std::string &chatId, const std::string &fromChatId,
		const int &messageId, const bool &disableNotification) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/forwardMessage?chat_id=" << chatId
	    << "&from_chat_id=" << fromChatId << "&message_id=" << messageId;

	if (disableNotification) url << "&disable_notification=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// editMessageText
api_types::Message tgbot::methods::Api::editMessageText(
		const std::string &chatId, const std::string &messageId,
		const std::string &text, const types::ParseMode &parseMode,
		const bool &disableWebPagePreview) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageText?chat_id=" << chatId
	    << "&message_id=" << messageId << "&text=";
	encode(url, text);

	if (parseMode == types::ParseMode::HTML)
		url << "&parse_mode=HTML";
	else if (parseMode == types::ParseMode::MARKDOWN)
		url << "&parse_mode=Markdown";

	if (disableWebPagePreview) url << "&disable_web_page_preview=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageText(
		const std::string &chatId, const std::string &messageId,
		const types::InlineKeyboardMarkup &replyMarkup, const std::string &text,
		const types::ParseMode &parseMode,
		const bool &disableWebPagePreview) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;

	url << baseApi << "/editMessageText?chat_id=" << chatId
	    << "&message_id=" << messageId << "&text=";
	encode(url, text);
	url << "&reply_markup=";
	encode(url, replyMarkup.toString());

	if (parseMode == types::ParseMode::HTML)
		url << "&parse_mode=HTML";
	else if (parseMode == types::ParseMode::MARKDOWN)
		url << "&parse_mode=Markdown";

	if (disableWebPagePreview) url << "&disable_web_page_preview=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageText(
		const std::string &inlineMessageId, const std::string &text,
		const types::ParseMode &parseMode,
		const bool &disableWebPagePreview) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageText?inline_message_id=" << inlineMessageId
	    << "&text=";
	encode(url, text);

	if (parseMode == types::ParseMode::HTML)
		url << "&parse_mode=HTML";
	else if (parseMode == types::ParseMode::MARKDOWN)
		url << "&parse_mode=Markdown";

	if (disableWebPagePreview) url << "&disable_web_page_preview=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageText(
		const std::string &inlineMessageId,
		const types::InlineKeyboardMarkup &replyMarkup, const std::string &text,
		const types::ParseMode &parseMode,
		const bool &disableWebPagePreview) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageText?inline_message_id=" << inlineMessageId
	    << "&text=";
	encode(url, text);
	url << "&reply_markup=";
	encode(url, replyMarkup.toString());

	if (parseMode == types::ParseMode::HTML)
		url << "&parse_mode=HTML";
	else if (parseMode == types::ParseMode::MARKDOWN)
		url << "&parse_mode=Markdown";

	if (disableWebPagePreview) url << "&disable_web_page_preview=true";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// editMessageCaption
api_types::Message tgbot::methods::Api::editMessageCaption(
		const std::string &chatId, const std::string &messageId,
		const std::string &caption) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageCaption?chat_id=" << chatId
	    << "&message_id=" << messageId << "&caption=";
	encode(url, caption);

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageCaption(
		const std::string &chatId, const std::string &messageId,
		const types::InlineKeyboardMarkup &replyMarkup,
		const std::string &caption) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageCaption?chat_id=" << chatId
	    << "&message_id=" << messageId << "&caption=";
	encode(url, caption);
	url << "&reply_markup=";
	encode(url, replyMarkup.toString());

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageCaption(
		const std::string &inlineMessageId, const std::string &caption) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageCaption?inline_message_id=" << inlineMessageId
	    << "&caption=";
	encode(url, caption);

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageCaption(
		const std::string &inlineMessageId,
		const types::InlineKeyboardMarkup &replyMarkup,
		const std::string &caption) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageCaption?inline_message_id=" << inlineMessageId
	    << "&caption=";
	encode(url, caption);
	url << "&reply_markup=";
	encode(url, replyMarkup.toString());

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// editMessageReplyMarkup
api_types::Message tgbot::methods::Api::editMessageReplyMarkup(
		const std::string &chatId, const std::string &messageId,
		const types::InlineKeyboardMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageReplyMarkup?chat_id=" << chatId
	    << "&message_id=" << messageId << "&reply_markup=";
	encode(url, replyMarkup.toString());

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageReplyMarkup(
		const std::string &inlineMessageId,
		const types::InlineKeyboardMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi
	    << "/editMessageReplyMarkup?inline_message_id=" << inlineMessageId
	    << "&reply_markup=";
	encode(url, replyMarkup.toString());

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendChatAction
bool tgbot::methods::Api::sendChatAction(
		const std::string &chatId, const types::ChatAction &action) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;

	url << baseApi << "/sendChatAction?chat_id=" << chatId << "&action=";
	if (action == types::ChatAction::TYPING)
		url << "typing";
	else if (action == types::ChatAction::FIND_LOCATION)
		url << "find_location";
	else if (action == types::ChatAction::RECORD_AUDIO)
		url << "record_audio";
	else if (action == types::ChatAction::RECORD_VIDEO)
		url << "record_video";
	else if (action == types::ChatAction::RECORD_VIDEO_NOTE)
		url << "record_video_note";
	else if (action == types::ChatAction::UPLOAD_AUDIO)
		url << "upload_audio";
	else if (action == types::ChatAction::UPLOAD_DOCUMENT)
		url << "upload_document";
	else if (action == types::ChatAction::UPLOAD_PHOTO)
		url << "upload_photo";
	else if (action == types::ChatAction::UPLOAD_VIDEO)
		url << "upload_video";
	else if (action == types::ChatAction::UPLOAD_VIDEO_NOTE)
		url << "upload_video_note";

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// sendContact
api_types::Message tgbot::methods::Api::sendContact(
		const std::string &chatId, const std::string &phoneNumber,
		const std::string &firstName, const std::string& vCard,
		const std::string &lastName,
		const bool &disableNotification, const int &replyToMessageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendContact?chat_id=" << chatId << "&phone_number=";
	encode(url, phoneNumber);
	url << "&first_name=";
	encode(url, firstName);

	if (!lastName.empty()) {
		url << "&last_name=";
		encode(url, lastName);
	}

	if (disableNotification) url << "&disable_notification=true";

	if (replyToMessageId != -1)
		url << "&reply_to_message_id=" << replyToMessageId;

	if (!vCard.empty()) {
		url << "&vcard=";
		encode(url, vCard);
	}

	const std::string &&markup = replyMarkup.toString();
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendGame
api_types::Message tgbot::methods::Api::sendGame(
		const int &chatId, const std::string &gameShortName,
		const bool &disableNotification, const int &replyToMessageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendGame?chat_id=" << chatId << "&game_short_name=";
	encode(url, gameShortName);

	if (disableNotification) url << "&disable_notification=true";

	if (replyToMessageId != -1) url << "&replyToMessageId=" << replyToMessageId;

	const std::string &&markup = replyMarkup.toString();
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendLocation
api_types::Message tgbot::methods::Api::sendLocation(
		const std::string &chatId, const double &latitude, const double &longitude,
		const int &liveLocation, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendLocation?chat_id=" << chatId
	    << "&latitude=" << latitude << "&longitude=" << longitude;

	if (liveLocation != -1) url << "&live_location=" << liveLocation;

	if (disableNotification) url << "&disable_notification=true";

	if (replyToMessageId != -1) url << "&replyToMessageId=" << replyToMessageId;

	const std::string &&markup = replyMarkup.toString();
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendVenue
api_types::Message tgbot::methods::Api::sendVenue(
		const std::string &chatId, const double &latitude, const double &longitude,
		const std::string &title, const std::string &address, const std::string &foursquareType,
		const std::string &foursquareId, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendVenue?chat_id=" << chatId << "&latitude=" << latitude
	    << "&longitude=" << longitude << "&title=";
	encode(url, title);
	url << "&address=";
	encode(url, address);

	if (!foursquareId.empty()) url << "&foursquare_id=" << foursquareId;

	if (!foursquareType.empty()) url << "&foursquare_type=" << foursquareType;

	if (disableNotification) url << "&disable_notification=true";

	if (replyToMessageId != -1) url << "&replyToMessageId=" << replyToMessageId;

	const std::string &&markup = replyMarkup.toString();
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendInvoice
api_types::Message tgbot::methods::Api::sendInvoice(
		const int &chatId, const types::Invoice &invoice,
		const bool &disableNotification, const int &replyToMessageId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendInvoice?chat_id=" << chatId;
	invoiceParams(url, invoice);

	if (disableNotification) url << "&disable_notification=true";

	if (replyToMessageId != -1) url << "&replyToMessageId=" << replyToMessageId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::sendInvoice(
		const int &chatId, const types::Invoice &invoice,
		const types::InlineKeyboardMarkup &replyMarkup,
		const bool &disableNotification, const int &replyToMessageId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/sendInvoice?chat_id=" << chatId << "&reply_markup=";
	encode(url, replyMarkup.toString());
	invoiceParams(url, invoice);

	if (disableNotification) url << "&disable_notification=true";

	if (replyToMessageId != -1) url << "&replyToMessageId=" << replyToMessageId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendVideo
api_types::Message tgbot::methods::Api::sendVideo(
		const std::string &chatId, const std::string &video,
		const types::FileSource &source, const std::string &mimeType,
		const int &duration, const int &width, const int &height,
		const std::string &caption, const bool &supportsStreaming,
		const bool &disableNotification, const int &replyToMessageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendVideo"
		    << "?chat_id=" << chatId << "&video=";

		encode(url, video);

		if (duration != -1) url << "&duration=" << duration;

		if (width != -1) url << "&width=" << width;

		if (height != -1) url << "&height=" << height;

		if (!caption.empty()) {
			url << "&caption=";
			encode(url, caption);
		}

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (supportsStreaming) url << "&supports_streaming=true";

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["video"] = http::value{nullptr, video.c_str(), mimeType.c_str()};

		if(duration != -1)
			forms["duration"] = http::value{std::to_string(duration).c_str(), nullptr, nullptr};

		if(width != -1)
			forms["width"] = http::value{std::to_string(width).c_str(), nullptr, nullptr};

		if(height != -1)
			forms["height"] = http::value{std::to_string(height).c_str(), nullptr, nullptr};

		if(!caption.empty())
			forms["caption"] = http::value{caption.c_str(), nullptr, nullptr};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};

		if(supportsStreaming)
			forms["supports_streaming"] = http::value{"true", nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/sendVideo", forms),
				value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendDocument
api_types::Message tgbot::methods::Api::sendDocument(
		const std::string &chatId, const std::string &document,
		const types::FileSource &source, const std::string &mimeType,
		const std::string &caption, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendDocument?chat_id=" << chatId << "&document=";

		encode(url, document);

		if (!caption.empty()) {
			url << "&caption=";
			encode(url, caption);
		}

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["document"] = http::value{nullptr, document.c_str(), mimeType.c_str()};

		if(!caption.empty())
			forms["caption"] = http::value{caption.c_str(), nullptr, nullptr};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};

		parseJsonObject(http::multiPartUpload(
				inst, baseApi + "/sendDocument", forms),
		                value);
	}

	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendPhoto
api_types::Message tgbot::methods::Api::sendPhoto(
		const std::string &chatId, const std::string &photo,
		const types::FileSource &source, const std::string &mimeType,
		const std::string &caption, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendPhoto?chat_id=" << chatId << "&photo=";

		encode(url, photo);

		if (!caption.empty()) {
			url << "&caption=";
			encode(url, caption);
		}

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["photo"] = http::value{nullptr, photo.c_str(), mimeType.c_str()};

		if(!caption.empty())
			forms["caption"] = http::value{caption.c_str(), nullptr, nullptr};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/sendPhoto", forms),
				value);
	}

	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendAudio
api_types::Message tgbot::methods::Api::sendAudio(
		const std::string &chatId, const std::string &audio,
		const types::FileSource &source, const std::string &mimeType,
		const std::string &caption, const int &duration,
		const std::string &performer, const std::string &title,
		const bool &disableNotification, const int &replyToMessageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendAudio?chat_id=" << chatId << "&audio=";

		encode(url, audio);

		if (!caption.empty()) {
			url << "&caption=";
			encode(url, caption);
		}

		if (!performer.empty()) {
			url << "&performer=";
			encode(url, performer);
		}

		if (!title.empty()) {
			url << "&title=";
			encode(url, title);
		}

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (duration != -1) {
			url << "&duration=" << duration;
		}

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["audio"] = http::value{nullptr, audio.c_str(), mimeType.c_str()};

		if(!caption.empty())
			forms["caption"] = http::value{caption.c_str(), nullptr, nullptr};

		if(duration != -1)
			forms["duration"] = http::value{std::to_string(duration).c_str(), nullptr, nullptr};

		if(!performer.empty())
			forms["performer"] = http::value{performer.c_str(), nullptr, nullptr};

		if(!title.empty())
			forms["title"] = http::value{title.c_str(), nullptr, nullptr};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/sendAudio", forms),
				value);
	}

	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendVoice
api_types::Message tgbot::methods::Api::sendVoice(
		const std::string &chatId, const std::string &voice,
		const types::FileSource &source, const std::string &caption,
		const int &duration, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendVoice?chat_id=" << chatId << "&voice=";
		encode(url, voice);

		if (duration != -1) url << "&duration=" << duration;

		if (!caption.empty()) {
			url << "&caption=";
			encode(url, caption);
		}

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {
		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["audio"] = http::value{nullptr, voice.c_str(), "audio/ogg"};

		if(!caption.empty())
			forms["caption"] = http::value{caption.c_str(), nullptr, nullptr};

		if(duration != -1)
			forms["duration"] = http::value{std::to_string(duration).c_str(), nullptr, nullptr};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};

		parseJsonObject(http::multiPartUpload(
				inst, baseApi + "/sendVoice", forms),
		                value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendSticker
api_types::Message tgbot::methods::Api::sendSticker(
		const std::string &chatId, const std::string &sticker,
		const types::FileSource &source, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendSticker?chat_id=" << chatId << "&sticker=";

		encode(url, sticker);

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {

		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["sticker"] = http::value{nullptr, sticker.c_str(), "image/png"};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};
		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/sendSticker", forms),
				value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// sendVideoNote
api_types::Message tgbot::methods::Api::sendVideoNote(
		const std::string &chatId, const std::string &videoNote,
		const types::FileSource &source, const std::string &caption,
		const int &duration, const bool &disableNotification,
		const int &replyToMessageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;
	const std::string &&markup = replyMarkup.toString();

	if (source == types::FileSource::EXTERNAL) {
		std::stringstream url;
		url << baseApi << "/sendVideoNote?chat_id=" << chatId << "&video_note=";

		encode(url, videoNote);

		if (duration != -1) url << "&duration=" << duration;

		if (!caption.empty()) {
			url << "&caption=";
			encode(url, caption);
		}

		if (disableNotification) url << "&disable_notification=true";

		if (replyToMessageId != -1)
			url << "&reply_to_message_id=" << replyToMessageId;

		if (!markup.empty()) {
			url << "&reply_markup=";
			encode(url, markup);
		}

		parseJsonObject(http::get(inst, url.str()), value);
	} else {

		http::PostForms forms;
		forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
		forms["video"] = http::value{nullptr, videoNote.c_str(), "video/mp4"};

		if(!caption.empty())
			forms["caption"] = http::value{caption.c_str(), nullptr, nullptr};

		if(duration != -1)
			forms["duration"] = http::value{std::to_string(duration).c_str(), nullptr, nullptr};

		if(disableNotification)
			forms["disable_notification"] = http::value{"true", nullptr, nullptr};

		if(replyToMessageId != -1)
			forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

		if(!markup.empty())
			forms["reply_markup"] = http::value{replyMarkup.toString().c_str(), nullptr, nullptr};

		parseJsonObject(
				http::multiPartUpload(inst, baseApi + "/sendVideoNote", forms),
				value);
	}
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// editMessageLiveLocation
api_types::Message tgbot::methods::Api::editMessageLiveLocation(
		const double &longitude, const double &latitude, const int &chatId,
		const int &messageId, const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageLiveLocation?longitude=" << longitude
	    << "&latitude=" << latitude << "&chat_id=" << chatId
	    << "&message_id=" << messageId;

	const std::string &markup{replyMarkup.toString()};
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::editMessageLiveLocation(
		const double &longitude, const double &latitude,
		const std::string &inlineMessageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageLiveLocation?longitude=" << longitude
	    << "&latitude=" << latitude << "&inline_message_id=" << inlineMessageId;

	const std::string &markup{replyMarkup.toString()};
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// stopMessageLiveLocation
api_types::Message tgbot::methods::Api::stopMessageLiveLocation(
		const int &chatId, const int &messageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/stopMessageLiveLocation?chat_id=" << chatId
	    << "&message_id=" << messageId;

	const std::string &markup{replyMarkup.toString()};
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message tgbot::methods::Api::stopMessageLiveLocation(
		const std::string &inlineMessageId,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi
	    << "/stopMessageLiveLocation?inline_message_id=" << inlineMessageId;

	const std::string &&markup{replyMarkup.toString()};
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

// setChatStickerSet
bool tgbot::methods::Api::setChatStickerSet(
		const int &chatId, const std::string &stickerSetName) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;

	url << baseApi << "/setChatStickerSet?chat_id=" << chatId
	    << "&sticker_set_name=" << stickerSetName;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

bool tgbot::methods::Api::setChatStickerSet(
		const std::string &chatId, const std::string &stickerSetName) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;

	url << baseApi << "/setChatStickerSet?chat_id=" << chatId
	    << "&sticker_set_name=" << stickerSetName;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

// deleteChatStickerSet
bool tgbot::methods::Api::deleteChatStickerSet(const int &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;

	url << baseApi << "/deleteChatStickerSet?chat_id=" << chatId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

bool tgbot::methods::Api::deleteChatStickerSet(
		const std::string &chatId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;

	url << baseApi << "/deleteChatStickerSet?chat_id=" << chatId;

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return true;
}

std::vector<api_types::Message> tgbot::methods::Api::sendMediaGroup(
		const std::string &chatId,
		const std::vector<tgbot::types::Ptr<types::InputMedia>> &media,
		const bool &disableNotification, const int &replyToMessageId) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	http::PostForms forms;
	forms["chat_id"] = http::value{chatId.c_str(), nullptr, nullptr};
	forms["media"] = http::value{arrayOfInputMediaSerializer(forms, media).c_str(), nullptr, nullptr};

	if(disableNotification)
		forms["disable_notification"] = http::value{"true", nullptr, nullptr};

	if(replyToMessageId != -1)
		forms["reply_to_message_id"] = http::value{std::to_string(replyToMessageId).c_str(), nullptr, nullptr};

	parseJsonObject(
			http::multiPartUpload(inst, baseApi + "/sendMediaGroup", forms),
			value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	std::vector<api_types::Message> messages;
	for (auto const &message : value.get("result", ""))
		messages.emplace_back(message);

	return messages;
}

api_types::Message tgbot::methods::Api::editMessageMedia(
		const std::string &inlineMessageId,
		const types::InputMedia &media,
		const types::ReplyMarkup &replyMarkup) const {

	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageMedia?inline_message_id=" << inlineMessageId
		<< "&media=";

	encode(url, media.toString());

	const std::string &markup{replyMarkup.toString()};
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}

api_types::Message
tgbot::methods::Api::editMessageMedia(
		const std::string &chatId,
		const int &messageId,
		const types::InputMedia &media,
		const types::ReplyMarkup &replyMarkup) const {
	CURL *inst = http::curlEasyInit();
	Json::Value value;

	std::stringstream url;
	url << baseApi << "/editMessageMedia?chat_id=" << chatId << "&message_id=" << messageId
		<< "&media=";

	encode(url, media.toString());

	const std::string &markup{replyMarkup.toString()};
	if (!markup.empty()) {
		url << "&reply_markup=";
		encode(url, markup);
	}

	parseJsonObject(http::get(inst, url.str()), value);
	curl_easy_cleanup(inst);

	if (!value.get("ok", "").asBool())
		throw TelegramException(value.get("description", "").asCString());

	return api_types::Message(value.get("result", ""));
}
