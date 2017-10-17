[![license](https://img.shields.io/github/license/StefanoBelli/telegram-bot-api.svg)](https://github.com/StefanoBelli/telegram-bot-api/blob/master/LICENSE)
[![GitHub tag](https://img.shields.io/github/tag/StefanoBelli/telegram-bot-api.svg)](https://github.com/StefanoBelli/telegram-bot-api/tags)
[![Travis](https://img.shields.io/travis/StefanoBelli/telegram-bot-api.svg)](https://travis-ci.org/StefanoBelli/telegram-bot-api)

# telegram-bot-api
C++ Telegram Bot APIs

### Documentation
Documentation is automatically generated by Doxygen and deployed by continuous integration tool after build succeed.

[TgBot documentation](http://stefanobelli.github.io/telegram-bot-api)

### Implementation status

 * Internal project utils [ directory: *include/tgbot/utils* ]
    
    * [x] HTTPS interface (libcurl wrapper)

 * Basic Bot interface [ files: *include/tgbot/bot.h* and *include/tgbot/register_callback.h* ]
    
    * [x] Bot class

    * [x] RegisterCallback class
    
 * API Types [ file: *include/tgbot/types.h* ]
 
    * [x]   User

	* [x]	ChatPhoto

	* [x]	MessageEntity
	
	* [x]	Audio
	
	* [x]	PhotoSize
	
	* [x]	Document
	
	* [x]	Voice
	
	* [x]	Contact
	
	* [x]	Location
	
	* [x]	Animation
	
	* [x]	Venue
	
	* [x]	VideoNote
	
	* [x]	MaskPosition
	
	* [x]	Sticker
	
	* [x]	StickerSet
	
	* [x]	Video
	
	* [x]	Invoice
	
	* [x]	ShippingAddress
	
	* [x]	OrderInfo
	
	* [x]	SuccessfulPayment
	
	* [x]	Game
	
	* [x]	Chat
	
	* [x]	Message
	
	* [x]	InlineQuery
	
	* [x]	ChosenInlineResult
	
	* [x]	CallbackQuery
	
	* [x]	ShippingQuery
	
	* [x]	PreCheckoutQuery
	
	* [x]	Update
	
	* [x]	ResponseParameters
	
	* [x]	File
	
	* [x]	UserProfilePhotos
	
	* [x]	KeyboardButton

	* [x]   ChatMember
	
 * API Methods and input types [ directory: *include/tgbot/methods* ]

	* [x]  ChatMemberRestrict 

	* [x]  ChatMemberPromote 

	* [x]  InlineKeyboardButton 

	* [x]  ReplyMarkup 

	* [x]  EmptyReplyMarkup 

	* [x]  InlineKeyboardMarkup 

	* [x]  ReplyKeyboardMarkup 

	* [x]  ReplyKeyboardRemove 

	* [x]  ForceReply 

	* [x]  InlineQueryResult 

	* [x]  InlineQueryResultAudio 

	* [x]  InlineQueryResultArticle 

	* [x]  InlineQueryResultContact 

	* [x]  InlineQueryResultGame 

	* [x]  InlineQueryResultDocument 

	* [x]  InlineQueryResultGif 

	* [x]  InlineQueryResultLocation 

	* [x]  InlineQueryResultMpeg4Gif 

	* [x]  InlineQueryResultPhoto 

	* [x]  InlineQueryResultVenue 

	* [x]  InlineQueryResultVideo 

	* [x]  InlineQueryResultVoice 
	
	* [x]  InlineQueryResultCachedAudio 

	* [x]  InlineQueryResultCachedDocument 

	* [x]  InlineQueryResultCachedGif 

	* [x]  InlineQueryResultCachedMpeg4Gif 

	* [x]  InlineQueryResultCachedPhoto 

	* [x]  InlineQueryResultCachedSticker 

	* [x]  InlineQueryResultCachedVideo 

	* [x]  InlineQueryResultCachedVoice 

	* [x]  InputMessageContent 

	* [x]  InputTextMessageContent 

	* [x]  InputLocationMessageContent 

	* [x]  InputContactMessageContent 

	* [x]  InputVenueMessageContent 

	* [x]  LabeledPrice 

	* [x]  Invoice 

	* [x]  ShippingOption 

 * API Methods

	* [ ]  addStickerToSet

	* [ ]  answerCallbackQuery

	* [ ]  answerInlineQuery

	* [ ]  answerPreCheckoutQuery

	* [ ]  answerShippingQuery

	* [ ]  createNewStickerSet

	* [ ]  deleteChatPhoto

	* [ ]  deleteMessage

	* [ ]  deleteStickerFromSet

	* [ ]  deleteWebhook

	* [ ]  editMessageCaption

	* [ ]  editMessageReplyMarkup

	* [ ]  editMessageText

	* [ ]  exportChatInviteLink

	* [ ]  forwardMessage

	* [ ]  getChat

	* [ ]  getChatAdministrators

	* [ ]  getChatMember

	* [ ]  getChatMembersCount

	* [ ]  getFile

	* [ ]  getGameHighScores

	* [ ]  getMe

	* [ ]  getStickerSet

	* [ ]  getUpdates

	* [ ]  getUpdatesvoid*

	* [ ]  getUserProfilePhotos

	* [ ]  getWebhookInfo

	* [ ]  kickChatMember

	* [ ]  leaveChat

	* [ ]  pinChatMessage

	* [ ]  promoteChatMember

	* [ ]  restrictChatMember

	* [ ]  sendAudio

	* [ ]  sendChatAction

	* [ ]  sendContact

	* [ ]  sendDocument

	* [ ]  sendGame

	* [ ]  sendInvoice

	* [ ]  sendLocation

	* [ ]  sendMessage

	* [ ]  sendPhoto

	* [ ]  sendSticker

	* [ ]  sendVenue

	* [ ]  sendVideo

	* [ ]  sendVideoNote

	* [ ]  sendVoice

	* [ ]  setChatDescription

	* [ ]  setChatPhoto

	* [ ]  setChatTitle

	* [ ]  setGameScore

	* [ ]  setStickerPositionInSet

	* [ ]  setWebhook

	* [ ]  unbanChatMember

	* [ ]  unpinChatMessage

	* [ ]  uploadStickerFile

