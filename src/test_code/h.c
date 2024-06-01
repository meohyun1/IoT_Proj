int turn(int* category) {
    if((dot_mtx = open(dot, O_RDWR)) < 0 ){
        printf("dot open error\n");
        exit(1);
    }

    int score[12] = {0};
    int roll_count = 0;
    unsigned char dip_input = 0;
    unsigned char tact_input = 0;
    unsigned char before_input = -1;
    int dice[5] = {0, 0, 0, 0, 0};

    while(1) {
        while(1) {
            usleep(10000); // 0.01 초 쉬기

            read(tactsw, &tact_input, sizeof(tact_input));
            // 한번 누른것으로 처리하기 위한 루프
            while(tact_input){
                unsigned char temp = 0;
                read(tactsw, &temp, sizeof(temp));
                if(!temp){
                    break;
                }
                usleep(10000); // 0.01 초 쉬기
                printf("%d",__LINE__);
            }

            // tactsw 입력 없고 3번 이하로 굴렸을 때
            if(!tact_input && roll_count < 3) {
                read(dipsw, &dip_input, sizeof(dip_input));
                // 딥스위치 맨 오른쪽 올렸을 때
                if(dip_input & 128) {
                    roll_dice(dice);
                    calc_score(score, dice);
                    set_lcd_bot(17);
                    roll_count++;
                    continue;
                }
            }

            // 입력 없을시 출력 없게
            // 아직 한번도 주사위 안굴렸으면 점수출력X
            if(roll_count == 0) {
                continue;
            }
            // tact 입력 없으면 아래 표시 할 필요 없음
            if(!tact_input) {
                continue;
            }
        }

        // 점수 출력 부분
        if(tact_input != before_input) {
            set_lcd_bot(tact_input-1);
            
            if((1 << (tact_input - 1)) & category) {
                set_turn_score(100);
            }
            else {
                set_turn_score(score[tact_input-1]);
                before_input = tact_input;
            }
        }
        // 점수 확정 부분
        else {
            //TODO category에 사용한 족보 넣기
            category |= 1 << (tact_input - 1);
            close(dot_mtx);
            return score[tact_input];
        }
    }
}
