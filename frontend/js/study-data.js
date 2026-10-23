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
            X('Dạng bài: tìm từ có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', ['dogs̲', 'books̲', 'maps̲'], 'dogs /dɒɡz/ đọc âm /z/; books /bʊks/ và maps /mæps/ đọc âm /s/.'),
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
            X('Dạng bài: tìm từ có NGUYÊN ÂM (phát âm) KHÁC hai từ còn lại.', 'Which word is different?', ['me̲a̲t', 'he̲a̲vy', 'bre̲a̲d'], 'meat /miːt/ có âm /iː/; heavy /ˈhevi/ và bread /bred/ đều có âm /e/.'),
            X('Dạng bài: tìm từ có PHÁT ÂM KHÁC hai từ còn lại.', 'Which word is different?', [' t̲h̲is', 't̲h̲ank', 't̲h̲ree'], 'this /ðɪs/ có âm /ð/; thank /θæŋk/ và three /θriː/ có âm /θ/.'),
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
            X('Dạng bài: tìm từ có NGUYÊN ÂM (phát âm) KHÁC hai từ còn lại.', 'Which word is different?', ['ca̲ke', 'ca̲t', 'ha̲t'], 'cat /kæt/ và hat /hæt/ có âm /æ/; cake /keɪk/ có âm /eɪ/.'),
            X('Dạng bài: tìm từ KHÁC NGHĨA (khác nhóm) với các từ còn lại.', 'Which word is different?', ['run', 'apple', 'banana'], 'apple và banana là trái cây; run là động từ "chạy".'),
            F("Tom ___ (not/like) fish.", ["doesn't like", "does not like"], 'Phủ định hiện tại đơn ngôi 3: does not + động từ nguyên mẫu.'),
            R('Lan gets up at six. She eats rice and goes to school by bike.', 'How does Lan go to school?', ['By bike', 'By bus', 'On foot'], '"goes to school by bike" = đi học bằng xe đạp.')]
    },
}