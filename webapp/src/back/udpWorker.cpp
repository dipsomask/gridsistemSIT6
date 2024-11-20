#include "settings.h"


class UDPWorkerTryingVectorsMultiplicationTasks {
public:
    UDPWorkerTryingVectorsMultiplicationTasks(io_context& io_context, int worker_port)
        : server_endpoint(udp::endpoint(udp::v4(), SERVER_PORT)), worker_endpoint(udp::endpoint(udp::v4(), worker_port)),
          socket_(io_context, udp::endpoint(udp::v4(), worker_port)) {
        finished = false;
        std::cout << "[Worker " << worker_endpoint.address() << ":" << worker_endpoint.port() << " started]\n";
        start_receive();
    }

private:
    udp::socket socket_;
    udp::endpoint worker_endpoint;
    udp::endpoint server_endpoint;
    udp::endpoint remote_endpoint;
    char recv_buffer[1024];
    bool finished;

    void start_receive() {

        if(finished){
            if(socket_.is_open()){
                socket_.close();
            }
            return;
        }

        socket_.async_receive_from(buffer(recv_buffer), remote_endpoint,
            [this](boost::system::error_code error, std::size_t bytes_transferred) {
                if (!error) {
                    handle_receive(bytes_transferred);
                }
                start_receive();
            });
    }

    void handle_receive(std::size_t length) {
        std::string message(recv_buffer, length);
        std::cout << "[Received message from server: " << message << "]" << std::endl;

        str answer = "";

        if(message != "Hello" && message != "Finish"){

            std::istringstream ss(message);
            std::string part;
            str indexes = part;

            std::getline(ss, part, '|');
            std::istringstream first_part(part);
            int row_index, column_index;
            first_part >> row_index >> column_index;

            std::getline(ss, part, '|');
            std::vector<double> left_vector = parse_vector(part);

            std::getline(ss, part);
            std::vector<double> right_vector = parse_vector(part);

            answer = indexes + " " + to_string(VectorMultiplication(left_vector, right_vector));

        }
        else if(message == "Hello"){
            answer = "Hello";
        }
        else if(message == "Finish"){
            finished = true;
            return;
        }

        send_message(answer);
    }

    std::vector<double> parse_vector(const std::string& str) {
        std::istringstream ss(str);
        std::vector<double> result;
        double value;
        
        while (ss >> value) {
            result.push_back(value);
            if (ss.peek() == ' ') {
                ss.ignore();
            }
        }
        
        return result;
    }

    double VectorMultiplication(vector<double>& left_vector, vector<double>& right_vector){

        double result = 0.0;
        for(double element_in_left_vector : left_vector){
            for(double element_in_right_vector : right_vector){
                result += element_in_left_vector * element_in_right_vector;
            }
        }
        return result;
    }

    void send_message(const std::string& message) {
        socket_.send_to(buffer(message), remote_endpoint);
        std::cout << "[Sent message to server: " << message << "]" << std::endl;
    }
};

int main(int argc, char* argv[]) {

    try {
        io_context io_context;
        int port = std::stoi(argv[1]);
        UDPWorkerTryingVectorsMultiplicationTasks worker(io_context, port);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
