
#include "settings.h"

using namespace settings;


class UDPServerDistributingMatrixMultiplicationTasks{

private:
    // 0 - not in process, 1 - finish, port - in process on potr
    std::unordered_map<str, str> IndexesOfElementsResultMatrixAndTheirState;
    unsigned int count_of_finished_tasks;
    unsigned int count_of_tasks_to_finish;
    vector<std::vector<double>> the_LeftMatrix;
    vector<std::vector<double>> the_RightMatrix;

    vector<vector<double>> the_ResultMatrix;
    unsigned int the_rows;
    unsigned int the_cols;

    std::unordered_map<udp::endpoint, std::chrono::steady_clock::time_point> Workers;
    int count_of_workers;

    udp::socket ServerSocket;
    udp::endpoint server_endpoint;

    udp::endpoint remote_endpoint;
    char recive_buffer[1024];
    bool finished;
    bool initialized;


protected:

    void SendTask(udp::endpoint& worker, str arguments){

        auto buffer = boost::asio::buffer(arguments.data(), arguments.size());
        ServerSocket.async_send_to(buffer, worker,
            [this, worker](boost::system::error_code error, std::size_t bytes_sent) {
                if (error) {
                    std::cerr << "Error sending message to worker: " << error.message() << std::endl;
                }
                udp::endpoint endpoint(worker);
                UpdateTimeut(endpoint);
            });

    }

    void WorkingWithAnswer(std::size_t lenth_of_recived_bytes, udp::endpoint worker){
        str answer = str(recive_buffer, lenth_of_recived_bytes);
        std::cout << answer << std::endl;

        if(answer != "Hello" && answer != ""){
            int row_index, column_index;
            double the_result_matrix_element;
            std::stringstream ss(answer);
            std::cout << "[" << answer << "]" << std::endl;
            ss >> row_index >> column_index >> the_result_matrix_element;

            the_ResultMatrix[row_index][column_index] = the_result_matrix_element;
            std::cout << "[" << the_result_matrix_element << " " << row_index << " " << column_index << "]" << std::endl;

            str Task = to_string(row_index) + " " + to_string(column_index);
            SetTaskStatus(Task, "1");
            count_of_finished_tasks++;
            if(count_of_finished_tasks == count_of_tasks_to_finish){

                std::cout << "[All tasks finished!]" << std::endl;
                std::cout << "[Result Matrix:" << std::endl;
                str result = "";
                for(int i = 0; i < the_rows; i++){
                    for(int j = 0; j < the_cols; j++){
                        result += to_string(the_ResultMatrix[i][j]) + " ";
                    }
                    result += "\n";
                }
                result += "]";
                std::cout << result;
                finished = true;
            }
        }
        else if(answer == "Hello"){
            for(int i = 0; i < workers_endpoints.size(); i++){
                if(workers_endpoints[i] == worker && (Workers.find(worker) == Workers.end())){
                    Workers[worker] = std::chrono::steady_clock::now();
                    count_of_workers++;
                    std::cout << "[" << worker.address() << ":" << worker.port() << " initialized],\n";
                    break;
                }
            }
            
        }
        
        if(count_of_workers == settings::workers_endpoints.size() && GetInitializedFlag() && !finished){

            str Task = SetTaskToWorkerAndReturnIt(to_string(worker.port()));

            if(Task != ""){
                SendTask(worker, Task);
            }
        }

    }

    void UpdateTimeut(udp::endpoint& worker){
        if(Workers.find(worker) != Workers.end()){
            Workers[worker] = std::chrono::steady_clock::now();
        }
    }

    void CheckAllTimeuts(){
        for(auto& worker : Workers){
            if(std::chrono::steady_clock::now() - worker.second > std::chrono::seconds(TIMEOUT_IN_SECONDS)){
                std::cout << "[" << "Timeout of waiting answer from:" << worker.first.address() << ":" << worker.first.port() << "]" << std::endl;
                udp::endpoint endpoint(worker.first);

                for(auto& key_pair : IndexesOfElementsResultMatrixAndTheirState){
                    if(key_pair.second == to_string(worker.first.port())){
                        key_pair.second = "0";
                        break;
                    }
                }
                
                str Task = SetTaskToWorkerAndReturnIt(to_string(worker.first.port()));

                SendTask(endpoint, Task);
            }
        }
    }

    void SetTaskStatus(str Task, str Status){
        if(IndexesOfElementsResultMatrixAndTheirState.find(Task) != IndexesOfElementsResultMatrixAndTheirState.end()){
            IndexesOfElementsResultMatrixAndTheirState[Task] = Status;
        }
    }

    str SetTaskToWorkerAndReturnIt(str port){
        str task;
        int row_index, column_index;
        
        for(auto& key_pair : IndexesOfElementsResultMatrixAndTheirState){
            if(key_pair.second == "0"){
                key_pair.second = port;
                task = key_pair.first;

                std::istringstream iss(task);
                iss >> row_index >> column_index;

                task += "|";
                for(int i = 0; i < the_cols; i++){
                    task += to_string(the_LeftMatrix[row_index][i]) + " ";
                }
                task += "|";
                for(int i = 0; i < the_rows; i++){
                    task += to_string(the_RightMatrix[i][column_index]) + " ";
                }

                return task;
            }
        }
        
        return "";

    }


public:
    UDPServerDistributingMatrixMultiplicationTasks(vector<std::vector<double>>& LeftMatrix, vector<std::vector<double>> RightMatrix,
                                                   io_context& io_context): the_LeftMatrix(LeftMatrix), the_RightMatrix(RightMatrix),
                                                   the_rows(LeftMatrix.size()), the_cols(RightMatrix[0].size()),
                                                   server_endpoint(udp::endpoint(udp::v4(), SERVER_PORT)),
                                                   ServerSocket(io_context, udp::endpoint(udp::v4(), SERVER_PORT)){
        
        finished = false;
        count_of_finished_tasks = 0;
        count_of_workers = 0;
        initialized = false;
        
        the_ResultMatrix.resize(the_rows);
        for(int i = 0; i < the_rows; i++){
            the_ResultMatrix[i].resize(the_cols);
            for(int j = 0; j < the_cols; j++){
                IndexesOfElementsResultMatrixAndTheirState[to_string(i) + " " + to_string(j)] = "0";
            }
        }

        count_of_tasks_to_finish = IndexesOfElementsResultMatrixAndTheirState.size();

        InitializeWorkers();
        ReciveMessages();

        for(auto& worker : Workers){
            
            udp::endpoint endpoint(worker.first);

            str Task = SetTaskToWorkerAndReturnIt(to_string(endpoint.port()));
            SendTask(endpoint, Task);
        }

    }

    void InitializeWorkers(){

        while(count_of_workers != settings::workers_endpoints.size()){

            for(auto& worker : settings::workers_endpoints){
                if(Workers.find(worker) == Workers.end()){
                    udp::endpoint endpoint(worker);
                    SendTask(endpoint, "Hello");
                    ReciveMessages();
                }
            }
        }

        SetInitializedFlag(true);

    }

    void StopWorkers(){
        for(auto& worker : Workers){
            udp::endpoint endpoint(worker.first);
            SendTask(endpoint, "Finish");
        }
        std::cout << "[Stopped workers:\n";
        for(const auto& worker : Workers){
            std::cout << "[" << worker.first.address() << ":" << worker.first.port() << " stopped],\n";
        }
        std::cout << "]\n";
        Workers.clear();
    }

    void ReciveMessages(){

        std::cout << "[Start recive messages]\n";

        if(finished){
            if(ServerSocket.is_open() && Workers.size() > 0){
                StopWorkers();
                ServerSocket.close();
            }
            return;
        }
        
        ServerSocket.async_receive_from(buffer(recive_buffer), remote_endpoint, 
            [this](boost::system::error_code error, std::size_t lenth_of_recived_bytes){
                std::cout << "[Go to callback function]\n";
                if(!error){
                    WorkingWithAnswer(lenth_of_recived_bytes, remote_endpoint);
                }
                else{
                    std::cout << "[ERROR:" << error.message() << "]" << std::endl;
                }

                if(GetInitializedFlag()){

                    CheckAllTimeuts();

                    ReciveMessages();

                }

            }
        );

    }

    vector<vector<double>>& GetResultMatrix(){
        return the_ResultMatrix;
    }

    int GetCountOfWorkers(){
        return count_of_workers;
    }

    bool GetInitializedFlag(){
        return initialized;
    }

    bool SetInitializedFlag(bool value){
        initialized = value;
        return initialized;
    }

};

int main(){

    io_context io_context;

    // Ввод размеров матриц
    unsigned int rows, cols;

    // Инициализация матриц
    vector<vector<double>> left_matrix = {
        {1.1, 2.2, 3.3},
        {4.4, 5.5, 6.6},
        {7.7, 8.8, 9.9}
    };
    vector<vector<double>> right_matrix = {
        {1.1, 2.2, 3.3},
        {4.4, 5.5, 6.6},
        {7.7, 8.8, 9.9}
    };

    // Создание и запуск сервера
    UDPServerDistributingMatrixMultiplicationTasks server(left_matrix, right_matrix, io_context);

    // Запуск цикла обработки событий
    io_context.run();

    return 0;

}