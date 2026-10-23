/*
 * NGÂN HÀNG CÂU HỎI CEFR A1 → C2: mỗi cấp 3 nhóm (easy/medium/hard); mỗi nhóm nên có 15 câu, mỗi lần làm rút ngẫu nhiên 5 câu. Cần đúng >= 4/5 để qua.
 * Hàm tạo câu (đáp án ĐÚNG của M/X/R luôn ghi đầu tiên trong mảng o, JS tự xáo trộn khi hiển thị):
 *  M(q, o, giải thích)           : chọn đáp án / điền chỗ trống có sẵn lựa chọn
 *  X(v, q, o, giải thích)        : tìm điểm KHÁC BIỆT (phát âm, trọng âm, nghĩa). v = đoạn tiếng Việt nói rõ khác ở điểm nào
 *  R(đoạn văn, q, o, giải thích) : đọc hiểu, tìm thông tin
 *  F(q, [đáp án...], giải thích) : tự gõ điền từ
 *  O(câu đúng, giải thích)       : sắp xếp từ thành câu
 */
const M = (q, o, e) => ({ t: 'mcq', q, o, e });
const X = (v, q, o, e) => ({ t: 'mcq', v, q, o, e });
const R = (x, q, o, e) => ({ t: 'read', x, q, o, e });
const F = (q, a, e) => ({ t: 'fill', q, a, e });
const O = (a, e) => ({ t: 'order', a, e });

const D = {
    A1: {
        easy: [
            R('Hello! My name is Ben. I am ten years old. I live in a small house with my parents and my sister. I like football, but my sister likes music.', 'Which statement is TRUE?', ['Ben likes football.', 'Ben lives with his friends.', 'Ben is twelve years old.'], 'Câu "I like football" cho biết Ben thích bóng đá.'),
            X('Dạng bài: tìm từ có TRỌNG ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['hotel', 'table', 'city'], 'hotel /hoʊˈtel/ nhấn âm 2; table và city nhấn âm 1.'),
            M('The elephant is big, but the mouse is ____.', ['small', 'tall', 'long'], 'So sánh: elephant (con voi) lớn, mouse (con chuột) nhỏ.'),
            M('The cat is ____ the table. It is directly underneath it.', ['under', 'on', 'behind'], 'Câu "It is directly underneath it" (ngay bên dưới cái bàn) cho thấy mèo đang ở dưới bàn.'),
            O('Lisa goes to school every day', 'Cấu trúc hiện tại đơn: S + V + O.'),
            X('Dạng bài: tìm từ s có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['dogs', 'books', 'maps'], 'dogs /dɒɡz/ đọc âm /z/; books /bʊks/ và maps /mæps/ đọc âm /s/.'),
            M('There are three ____ on the table.', ['books', 'book', 'bookes'], 'Từ khóa "three" là số nhiều.Số nhiều của "book" là "books".'),
            M('Tom and I are friends. ____ are in the same class.', ['We', 'They', 'He'], '"Tom and I" gồm người nói và người khác nên dùng "We"(chúng tôi/chúng ta).'),
            M('I ____ a student. My sister ____ a teacher.', ['am/is', 'is/am', 'are/are'], 'Chủ ngữ "I" đi với "am", chủ ngữ "My sister" đi với "is".'),
            M('My mother is a ____. She works in a hospital and helps sick people.', ['nurse', 'farmer', 'teacher'], 'Nurse nghĩa là y tá/điều dưỡng. Từ khóa hospital (bệnh viện) và helps sick people (giúp người bệnh) gợi ý nghề nghiệp này. Teacher: giáo viên. Farmer: nông dân.'),
            M('I ___ a student.', ['am', 'is', 'are'], 'Chủ ngữ "I" luôn đi với "am": I am a student.'),
            M('She ___ my sister.', ['is', 'am', 'are'], 'He/She/It đi với "is".'),
            M('This is ___ apple.', ['an', 'a', 'the'], 'Trước nguyên âm (a, e, i, o, u) dùng "an": an apple.'),
            M('Which word is a color?', ['red', 'dog', 'run'], '"red" (đỏ) là màu; dog là con chó, run là chạy.'),
            M('___ is your name?', ['What', 'Where', 'Who'], 'Hỏi tên dùng "What": What is your name?')],
        medium: [
            M('A: Would you like some tea? B: _____', ['Yes, please.', 'No, I am tea.', 'Yes, I do yesterday.'], 'Câu trả lời "Yes, please" (vâng, cảm ơn) phù hợp với câu hỏi "Would you like some tea?" (Bạn có muốn uống trà không?).'),
            X('Dạng bài: tìm từ ea có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['meat', 'heavy', 'bread'], 'meat /miːt/ có âm /iː/; heavy /ˈhevi/ và bread /bred/ đều có âm /e/.'),
            X('Dạng bài: tìm từ th có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', [' this', 'thank', 'three'], 'this /ðɪs/ có âm /ð/; thank /θæŋk/ và three /θriː/ có âm /θ/.'),
            M('Shop assistant: Can I help you?. Customer: _____', ['Yes, I want to buy a T-shirt.', 'No, I am fine.', 'I like pizza.'], 'Câu trả lời "Yes, I want to buy a T-shirt" (vâng, tôi muốn mua một cái áo phông) phù hợp với câu hỏi "Can I help you?" (Tôi có thể giúp gì cho bạn?).'),
            X('Dạng bài: Chọn câu có nghĩa TƯƠNG DƯƠNG', 'Peter is younger than John.', ['John is older than Peter.', 'John is younger than Peter.', 'Peter is older than John.'], 'Câu "Peter is younger than John" (Peter trẻ hơn John) tương đương với "John is older than Peter" (John già hơn Peter).'),
            O('What time do you get up?', 'Câu hỏi hiện tại đơn: What + do + S + V.'),
            X('Dạng bài: Tìm lỗi sai. Chọn phần sai trong câu', 'She don\'t like apples', ['don\'t', 'like', 'apples'], 'Câu sai: "don\'t". Câu đúng: "She doesn\'t like apples" (ngôi thứ ba số ít ở hiện tại đơn).'),
            M('My English class starts ______ 7:30 a.m.', ['on', 'in', 'at'], 'Thời gian cụ thể dùng "at": at 7:30 a.m.'),
            M('There isn\'t ____ milk in the fridge.', ['any', 'some', 'a'], 'Câu phủ định với danh từ không đếm được dùng "any".'),
            M('A: ______ do you live?. B: I live in Hanoi.', ['Where', 'What', 'When'], 'Hỏi nơi chốn dùng "Where": Where do you live?'),
            M('My father ____ coffee every morning.', ['drinks', 'drink', 'drinking'], 'Ngôi thứ ba số ít ở hiện tại đơn: drink → drinks.'),
            F('She ___ (have) two cats.', ['has'], 'Ngôi thứ ba số ít ở hiện tại đơn: have → has.'),
            O('My name is Anna', 'Cấu trúc giới thiệu tên: My name is + tên.'),
            M('They ___ at school now.', ['are', 'is', 'am'], '"They" đi với "are".'),
            F('There ___ (be) a book on the desk.', ['is'], 'There is + danh từ số ít; There are + danh từ số nhiều.'),
            M('I ___ breakfast at 7 a.m.', ['have', 'has', 'having'], 'Chủ ngữ "I" dùng "have" ở hiện tại đơn.')],
        hard: [
            R('David works in a small shop from Monday to Friday. He starts work at 8 a.m. and finishes at 4 p.m. On Saturdays, he doesn\'t work.He usually visits his grandmother, who lives near his house. However, this Saturday, he is going to the beach with his friends.', 'Which statement is TRUE?', ['David usually visits his grandmother on Saturdays.', 'David finishes work at 8 p.m', 'David works every day.'], 'Câu "On Saturdays, he doesn\'t work. He usually visits his grandmother" cho biết David thường đến thăm bà vào các ngày thứ Bảy.'),
            X('Dạng bài: dịch câu Tiếng Việt sang Tiếng Anh', 'Cô ấy không có anh chị em.', ['She doesn\'t have any brothers or sisters.', 'She don\'t have any brothers or sisters.', 'She has not any brothers or sisters.'], 'Câu phủ định hiện tại đơn và chủ ngữ là "She" nên dùng "does\'n": She doesn\'t have any brothers or sisters.'),
            R('Đọc đoạn tin nhắn sau: "Hi Lucy, I\'m sorry, but I can\'t meet you at the café at 3 p.m. My English lesson finishes at 3: 30. Can we meet at the library at 4 p.m.instead ?"_ Sam', 'Why can\'t Sam meet Lucy at 3 p.m.? ', ['He has an English lesson.', 'He doesn\'t like cafés', 'He is going home at 3 p.m.'], 'Câu "My English lesson finishes at 3:30" cho biết Sam không thể gặp Lucy lúc 3 giờ vì bài học tiếng Anh kết thúc lúc 3:30.'),
            X('Dạng bài: tìm từ có TRỌNG ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['visit', 'relax', 'forget'], 'visit /ˈvɪzɪt/ nhấn âm 1; relax /rɪˈlæks/ và forget /fərˈɡet/ đều nhấn âm 2.'),
            M('A:____. B: Because I am tired.', ['Why are you going to bed early?', 'Who is tired?', 'How are you?'], 'Câu trả lời "Because I am tired" (Vì tôi mệt) phù hợp với câu hỏi "Why are you going to bed early?" (Tại sao bạn đi ngủ sớm?).'),
            M('Emma usually walks to school, but today it is raining heavily. She ______ the bus.', ['is taking', 'takes', 'took'], 'Hành động đang diễn ra: hiện tại tiếp diễn (am/is/are + V-ing).'),
            X('Dạng bài: Tìm lỗi sai. Chọn phần sai trong câu', 'There are a book on the desk.', ['are', 'on', 'the'], 'Câu sai: "are". Câu đúng: "There is a book on the desk." (dùng "is" với danh từ số ít).'),
            M('How ______ apples and how ______ water do we need?', ['many/much', 'much/many', 'few/little'], '"How many" dùng với danh từ đếm được (apples); "How much" dùng với danh từ không đếm được (water).'),
            M('I want ______ English because I want to talk to people from other countries.', ['to learn', 'learning', 'learn'], 'want + to V: muốn làm gì.'),
            M('This is Anna\'s bag.It is ______ bag.', ['her', 'she\'s', 'hers'], '"her" là tính từ chỉ sở hữu.'),
            M('Look! The children ______ in the garden now.', ['are playing', 'is play', 'played'], 'Hành động đang diễn ra: hiện tại tiếp diễn (am/is/are + V-ing).'),
            R('Tom is ten. He has a dog. The dog is brown. They play in the park.', 'What color is the dog?', ['Brown', 'Black', 'White'], 'Câu "The dog is brown" nói rõ chó màu nâu.'),
            X('Dạng bài: tìm từ a có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['cake', 'cat', 'hat'], 'cat /kæt/ và hat /hæt/ đều có âm /æ/; cake /keɪk/ có âm /eɪ/.'),
            X('Dạng bài: tìm từ KHÁC NGHĨA (khác nhóm) với các từ còn lại.', 'Which word is different?', ['run', 'apple', 'banana'], 'apple và banana là trái cây; run là động từ "chạy".'),
            F("Tom ___ (not/like) fish.", ["doesn't like", "does not like"], 'Phủ định hiện tại đơn ngôi 3: does not + động từ nguyên mẫu.'),
            R('Lan gets up at six. She eats rice and goes to school by bike.', 'How does Lan go to school?', ['By bike', 'By bus', 'On foot'], '"goes to school by bike" = đi học bằng xe đạp.')]
    },
    A2: {
        easy: [
            R('Emma went to the seaside with her family last Sunday. They travelled by train and arrived at 10 a.m. The weather was warm, so they swam in the sea and ate lunch at a small restaurant. They returned home at 7 p.m.', 'How did Emma and her family travel to the seaside?', ['By train', 'By bus', 'By car'], 'Câu "They travelled by train" cho biết họ đi bằng tàu hỏa.'),
            X('Dạng bài: tìm từ có TRỌNG ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['begin', 'city', 'country'], ' begin /bɪˈɡɪn/ nhấn âm 2; city /ˈsɪti/ và country /ˈkʌntri/ đều nhấn âm 1.'),
            M('A: What were you doing at 8 p.m. last night? B: ______', ['I was watching TV.', 'I watch TV.', 'I watched TV.'], ' Thì quá khứ tiếp diễn: hành động đang diễn ra tại một thời điểm trong quá khứ (was/were + V-ing).'),
            M('There are too ______ people on this bus. We can\'t find any seats.', ['many', 'much', 'few'], '"too many" dùng với danh từ đếm được (people).'),
            M('A bicycle is usually ______ than a car.', ['cheaper', 'cheap', 'cheapest'], 'So sánh hơn với tính từ ngắn: cheap + er + than.'),
            O('I visited my grandparents last weekend.', 'Câu quá khứ đơn: S + V-ed + O.'),
            X('Dạng bài: tìm từ oo có PHÁT ÂM KHÁC hai từ còn lại.', ['book', 'school', 'food'], 'book /bʊk/ có âm /ʊ/; school /skuːl/ và food /fuːd/ đều có âm /uː/.'),
            X('Dạng bài: tìm từ ed có PHÁT ÂM KHÁC hai từ còn lại.', ['needed', 'walked', 'stopped'], 'needed /ˈniːdɪd/ đọc âm /ɪd/; walked /wɔːkt/ và stopped /stɒpt/ đều đọc âm /t/.'),
            M('Chọn Câu Đúng:', ['She didn\'t go to school yesterday', 'She don\'t go to school yesterday', 'She doesn\'t went to school yesterday'], 'Quá khứ đơn: She + didn\'t + V nguyên mẫu.'),
            M('You should take an umbrella. It is going to ______.', ['rain', 'snowman', 'cloudy weather'], 'Từ khóa "take an umbrella" là mang theo ô, going to + V: dự đoán thời tiết; rain = mưa, snowman = người tuyết, cloudy weather = trời nhiều mây.'),
            M('My brother is interested ______ learning English.', ['in', 'on', 'for'], 'Cấu trúc: be interested in + V-ing.'),
            M('Yesterday, we ______ to the cinema and watched a funny film.', ['went', 'go', 'gone'], '"Yesterday" cần quá khứ đơn; go → went (bất quy tắc).'),
            M('Yesterday I ___ to the market.', ['went', 'go', 'goes'], '"Yesterday" cần quá khứ đơn; go → went (bất quy tắc).'),
            M('She is ___ than her sister.', ['taller', 'tall', 'tallest'], 'So sánh hơn với tính từ ngắn: tall + er + than.'),
            M("I'm going to ___ my grandma tomorrow.", ['visit', 'visited', 'visiting'], 'be going to + động từ nguyên mẫu, diễn tả dự định.'),
            M("There isn't ___ milk in the fridge.", ['any', 'some', 'a'], 'Câu phủ định với danh từ không đếm được dùng "any".'),
            M('We ___ football every Sunday.', ['play', 'plays', 'played'], '"every Sunday" → hiện tại đơn; chủ ngữ "We" giữ động từ nguyên mẫu.')],
        medium: [
            M('A: Would you mind opening the window? B: ______', ['Not at all.', 'I am opening yesterday.', 'i\'m fine,thank you'], 'Câu trả lời "Not at all" (Không có gì) phù hợp với câu hỏi "Would you mind opening the window?" (Bạn có phiền mở cửa sổ không?).'),
            M('The train arrives ______ the station at 6:15 p.m.', ['at', 'in', 'on'], 'Thời gian cụ thể dùng "at": at 6:15 p.m.'),
            X('Dạng bài: Chọn câu có nghĩa TƯƠNG DƯƠNG', 'Although it was raining, we went for a walk.', ['it was raining, but we went for a walk', 'We went for a walk because it was raining', 'We didn\'t go for a walk because of the rain.'], 'Câu "Although it was raining, but we went for a walk" (Mặc dù trời mưa, chúng tôi vẫn đi dạo) tương đương với "It was raining, but we went for a walk" (Trời mưa nhưng chúng tôi vẫn đi dạo).'),
            X('Dạng bài: Tìm lỗi sai. Chọn phần sai trong câu', 'There is too many sugar in my coffee.', ['many', 'there', 'in'], 'Câu sai: "many" vì sugar (đường) là danh từ không đếm được. Câu đúng: "There is too much sugar in my coffee." (dùng "too much" với danh từ không đếm được).'),
            M('A: ______ have you lived in this town?. B: For three years.', ['How long', 'How many', 'How often'], 'Hỏi khoảng thời gian dùng "How long": How long have you lived in this town?'),
            M('We ______ visit our cousins this weekend. We have already bought the train tickets.', ['are going to', 'will', 'went to'], 'be going to + V: dự định trong tương lai.'),
            M('You ______ wear a seat belt when you travel by car. It is the law.', ['have to', 'should to', 'might'], 'Từ khóa "law" là luật, have to diễn tả sự bắt buộc/luật lệ.'),
            M('Mount Everest is ______ mountain in the world.', ['the highest', 'higher', 'high'], 'So sánh nhất với tính từ dài: the + adj + est.'),
            F('He ___ (watch) TV when I called.', ['was watching'], 'Hành động đang diễn ra thì bị cắt ngang: quá khứ tiếp diễn (was/were + V-ing).'),
            O('I have never eaten sushi', 'Hiện tại hoàn thành: have + never + V3 (eaten) để nói về trải nghiệm.'),
            M('How ___ water do you drink a day?', ['much', 'many', 'few'], 'water là danh từ không đếm được → "How much".'),
            F('I ___ (see) him last week.', ['saw'], '"last week" → quá khứ đơn; see → saw.'),
            M('This book is ___ than that one.', ['more interesting', 'interestinger', 'most interesting'], 'Tính từ dài so sánh hơn: more + adj + than.')],
        hard: [
            R('Laura moved to a new town six months ago. At first, she didn\'t know anyone and found it difficult to travel around.She joined a local photography club in April and made several friends there.Now she often goes cycling with them at weekends.She hasn\'t visited the town\'s art museum yet, but she plans to go there next Saturday.', 'Which statement is TRUE?', ['Laura plans to visit the art museum next Saturday.', 'Laura has already visited the art museum.', 'Laura doesn\'t have any friends in the new town.'], 'Câu "she plans to go there next Saturday" cho biết Laura dự định đến thăm bảo tàng nghệ thuật vào thứ Bảy tới.'),
            R('Đọc đoạn tin nhắn sau: "Hi Jack, The football match on Saturday has been moved from 3 p.m.to 5 p.m.because the stadium needs more time to prepare.I can still meet you outside the main entrance at 4: 30. Don\'t forget to bring your ticket!"_Mike', 'Why has the match been moved?', ['The stadium needs more time to prepare.', 'The match has been cancelled.', 'Jack forgot his ticket.'], 'Câu "because the stadium needs more time to prepare" (vì sân vận động cần thêm thời gian chuẩn bị) cho biết lý do trận đấu bị dời giờ.'),
            X('Dạng bài: tìm từ s có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['plays', 'laughs', 'sits'], 'plays /pleɪz/ đọc âm /z/; laughs /læfs/ và sits /sɪts/ đều đọc âm /s/.'),
            X('Dạng bài: Chọn câu diễn đạt đúng', 'Bạn muốn nói rằng trước đây mình không thể bơi, nhưng hiện tại đã biết bơi.', ['I couldn\'t swim before, but now I can.', 'I can swim before, but now I couldn\'t.', 'I can swim before, but now I can.'], 'Câu đúng: "I couldn\'t swim before, but now I can." (Trước đây tôi không thể bơi, nhưng bây giờ tôi có thể.).'),
            X('Dạng bài: Tìm lỗi sai. Chọn phần sai trong câu', 'He has visited London last year.', ['has', 'visited', 'last'], 'Câu sai: "has" vì "last year" là mốc thời gian trong quá khứ → dùng quá khứ đơn: He visited London last year.'),
            M('The woman ______ lives next door is a doctor.', ['who', 'whom', 'where'], 'Mệnh đề quan hệ xác định cho người (woman) thay thế bằng đại từ quan hệ "who".'),
            M('I enjoy ______ books in my free time.', ['reading', 'to read', 'read'], 'enjoy + V-ing: thích làm gì.'),
            M('If it ______ tomorrow, we\'ll stay at home.', ['rains', 'raining', 'will rain'], 'Câu điều kiện loại 1: If + hiện tại đơn, will + V nguyên mẫu.'),
            M('I ______ my homework, so I can go out now.', ['have finished', 'finished', 'finish'], 'Hiện tại hoàn thành: have + V3 (finished) với mốc thời gian không xác định.'),
            R('Last weekend, Linh went to the beach with her family. The weather was sunny, so they swam and built sandcastles. They came home late in the evening.', 'How was the weather?', ['Sunny', 'Rainy', 'Cold'], 'Câu "The weather was sunny" cho biết trời nắng.'),
            X('Dạng bài: tìm từ có TRỌNG ÂM (nhấn âm) rơi vào ÂM TIẾT KHÁC các từ còn lại.', 'Which word is different?', ['hotel', 'table', 'city'], 'hotel /hoʊˈtel/ nhấn âm 2; table và city nhấn âm 1.'),
            X('Dạng bài: tìm từ KHÁC NGHĨA (khác nhóm) với các từ còn lại.', 'Which word is different?', ['pencil', 'doctor', 'nurse'], 'doctor và nurse là nghề nghiệp; pencil là đồ vật.'),
            F('If it rains, we ___ (stay) at home.', ['will stay', "'ll stay"], 'Câu điều kiện loại 1: If + hiện tại đơn, will + V.'),
            R('Minh wants to buy a jacket. The red one costs $30 and the blue one costs $25. He chooses the cheaper one.', 'Which jacket does Minh buy?', ['The blue one', 'The red one', 'Both'], 'Blue ($25) rẻ hơn red ($30); "the cheaper one" là áo xanh.')]
    },
}